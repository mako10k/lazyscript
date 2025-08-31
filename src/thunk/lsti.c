#include "lsti.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "thunk.h"
#include "common/malloc.h"
#include "runtime/trace.h"
#include "thunk_bin.h"
#include <stddef.h>

#ifndef ENABLE_LSTI
// If not enabled, provide stubs that return ENOSYS to keep build/link stable.
int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc, const lsti_write_opts_t* opt) {
  (void)fp; (void)roots; (void)rootc; (void)opt; return -ENOSYS;
}
int lsti_map(const char* path, lsti_image_t* out_img) {
  (void)path; (void)out_img; return -ENOSYS;
}
int lsti_validate(const lsti_image_t* img) {
  (void)img; return -ENOSYS;
}
int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc, struct lstenv* prelude_env) {
  (void)img; (void)out_roots; (void)out_rootc; (void)prelude_env; return -ENOSYS;
}
int lsti_unmap(lsti_image_t* img) {
  (void)img; return -ENOSYS;
}

#else

typedef struct lsti_header_disk {
  uint32_t magic;          // 'LSTI'
  uint16_t version_major;  // =1
  uint16_t version_minor;  // =0
  uint16_t section_count;
  uint16_t align_log2;     // 3 or 4
  uint32_t flags;          // LSTI_F_*
} lsti_header_disk_t;

typedef struct lsti_section_disk {
  uint32_t kind;     // lsti_sect_kind_t
  uint32_t reserved; // 0
  uint64_t file_off; // absolute file offset
  uint64_t size;     // payload size in bytes
} lsti_section_disk_t;

static int fpad_to_align(FILE* fp, uint16_t align_log2) {
  long pos = ftell(fp);
  if (pos < 0) return -EIO;
  long mask = (1L << align_log2) - 1L;
  long pad = ((pos + mask) & ~mask) - pos;
  static const uint8_t zeros[16] = {0};
  while (pad > 0) {
    size_t chunk = (size_t)((pad > (long)sizeof(zeros)) ? sizeof(zeros) : pad);
    if (fwrite(zeros, 1, chunk, fp) != chunk) return -EIO;
    pad -= (long)chunk;
  }
  return 0;
}

int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc, const lsti_write_opts_t* opt) {
  if (!fp || rootc < 0) return -EINVAL;
  uint16_t align_log2 = opt ? opt->align_log2 : (uint16_t)LSTI_ALIGN_8;
  if (align_log2 != (uint16_t)LSTI_ALIGN_8 && align_log2 != (uint16_t)LSTI_ALIGN_16) return -EINVAL;
  if ((uint64_t)rootc > 0xFFFFFFFFu) return -EOVERFLOW;

  // -----------------
  // Build graph and pools (minimal): support INT/STR/SYMBOL/ALGE/BOTTOM
  // -----------------
  typedef struct vec_th {
    lsthunk_t** data; lssize_t size, cap;
  } vec_th_t;
  typedef struct vec_u32 { uint32_t* data; lssize_t size, cap; } vec_u32_t;
  typedef struct vec_u8  { uint8_t*  data; lssize_t size, cap; } vec_u8_t;
  typedef struct pool_ent { const char* bytes; lssize_t len; uint32_t off; } pool_ent_t;
  typedef struct vec_pool { pool_ent_t* data; lssize_t size, cap; } vec_pool_t;

  #define VEC_PUSH(vec, val, type) do { \
    if ((vec).size == (vec).cap) { lssize_t ncap = (vec).cap ? (vec).cap*2 : 8; \
      void* np = realloc((vec).data, (size_t)ncap * sizeof(type)); if (!np) return -ENOMEM; \
      (vec).data = (type*)np; (vec).cap = ncap; } \
    (vec).data[(vec).size++] = (val); \
  } while(0)

  vec_th_t nodes = {0};
  vec_u32_t edges = {0}; // flattened child id list; we'll write per-node later
  vec_pool_t spool = {0}; // string blob: STR values and BOTTOM messages
  vec_pool_t ypool = {0}; // symbol blob: SYMBOL literals and ALGE constructors

  // naive ID lookup helper (inline loop where needed)
  #define GET_ID(VAR_T, OUT_ID) do { \
    int __found = -1; \
    for (lssize_t __i = 0; __i < nodes.size; ++__i) { if (nodes.data[__i] == (VAR_T)) { __found = (int)__i; break; } } \
    (OUT_ID) = __found; \
  } while(0)

  // add to string pool if not exists; return index
  #define ADD_SPOOL(BYTES, LEN, OUT_IDX) do { \
    int __idx = -1; \
    for (lssize_t __i = 0; __i < spool.size; ++__i) { \
      if (spool.data[__i].len == (LEN) && memcmp(spool.data[__i].bytes, (BYTES), (size_t)(LEN)) == 0) { __idx = (int)__i; break; } \
    } \
    if (__idx < 0) { pool_ent_t __e; __e.bytes = (BYTES); __e.len = (LEN); __e.off = 0u; VEC_PUSH(spool, __e, pool_ent_t); __idx = (int)(spool.size - 1); } \
    (OUT_IDX) = __idx; \
  } while(0)

  // add to symbol pool if not exists; return index
  #define ADD_YPOOL(LSSTR, OUT_IDX) do { \
    const lsstr_t* __s = (LSSTR); \
    int __idx = -1; \
    if (__s) { \
      const char* __b = lsstr_get_buf(__s); lssize_t __l = lsstr_get_len(__s); \
      for (lssize_t __i = 0; __i < ypool.size; ++__i) { \
        if (ypool.data[__i].len == __l && memcmp(ypool.data[__i].bytes, __b, (size_t)__l) == 0) { __idx = (int)__i; break; } \
      } \
      if (__idx < 0) { pool_ent_t __e; __e.bytes = __b; __e.len = __l; __e.off = 0u; VEC_PUSH(ypool, __e, pool_ent_t); __idx = (int)(ypool.size - 1); } \
    } \
    (OUT_IDX) = __idx; \
  } while(0)
  // enqueue
  lssize_t qh = 0;
  for (lssize_t i = 0; i < rootc; ++i) {
  if (roots) { int __id; GET_ID(roots[i], __id); if (__id < 0) VEC_PUSH(nodes, roots[i], lsthunk_t*); }
  }
  while (qh < nodes.size) {
    lsthunk_t* t = nodes.data[qh++];
    switch (lsthunk_get_type(t)) {
      case LSTTYPE_INT: {
        (void)lsint_get(lsthunk_get_int(t));
        break;
      }
      case LSTTYPE_STR: {
        const lsstr_t* s = lsthunk_get_str(t);
  if (s) { int __idx; ADD_SPOOL(lsstr_get_buf(s), lsstr_get_len(s), __idx); (void)__idx; }
        break;
      }
      case LSTTYPE_SYMBOL: {
        const lsstr_t* sym = lsthunk_get_symbol(t);
  if (sym) { int __idx; ADD_YPOOL(sym, __idx); (void)__idx; }
        break;
      }
      case LSTTYPE_ALGE: {
        const lsstr_t* c = lsthunk_get_constr(t);
  if (c) { int __idx; ADD_YPOOL(c, __idx); (void)__idx; }
        lssize_t ac = lsthunk_get_argc(t);
        lsthunk_t* const* as = lsthunk_get_args(t);
        for (lssize_t i = 0; i < ac; ++i) {
          lsthunk_t* ch = as[i];
          int id; GET_ID(ch, id); if (id < 0) { VEC_PUSH(nodes, ch, lsthunk_t*); id = (int)(nodes.size-1); }
          (void)id; // recorded later per-node
        }
        break;
      }
      case LSTTYPE_BOTTOM: {
        const char* msg = lsthunk_bottom_get_message(t);
  if (msg) { int __idx; ADD_SPOOL(msg, (lssize_t)strlen(msg), __idx); (void)__idx; }
        lssize_t ac = lsthunk_bottom_get_argc(t);
        lsthunk_t* const* as = lsthunk_bottom_get_args(t);
        for (lssize_t i = 0; i < ac; ++i) {
          lsthunk_t* ch = as[i];
          int id; GET_ID(ch, id); if (id < 0) { VEC_PUSH(nodes, ch, lsthunk_t*); id = (int)(nodes.size-1); }
          (void)id;
        }
        break;
      }
      default:
        // Unsupported in Phase 1
        // Clean up
        free(nodes.data); free(spool.data); free(ypool.data);
        return -ENOTSUP;
    }
  }

  // -----------------
  // Write header + section table placeholders (we may include 2 optional pools)
  // -----------------
  int sect_count = 2; // THUNK_TAB + ROOTS
  int has_spool = (spool.size > 0);
  int has_ypool = (ypool.size > 0);
  if (has_spool) ++sect_count;
  if (has_ypool) ++sect_count;
  // Header
  lsti_header_disk_t hdr;
  hdr.magic = LSTI_MAGIC;
  hdr.version_major = LSTI_VERSION_MAJOR;
  hdr.version_minor = LSTI_VERSION_MINOR;
  hdr.section_count = (uint16_t)sect_count;
  hdr.align_log2 = align_log2;
  hdr.flags = opt ? opt->flags : 0u;
  if (fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) return -EIO;
  // Placeholder section table
  long sect_tbl_off = ftell(fp);
  if (sect_tbl_off < 0) return -EIO;
  for (int i = 0; i < sect_count; ++i) {
    lsti_section_disk_t zero = {0};
    if (fwrite(&zero, 1, sizeof(zero), fp) != sizeof(zero)) return -EIO;
  }

  // Write STRING_BLOB
  lsti_section_disk_t string_entry = {0};
  if (has_spool) {
    if (fpad_to_align(fp, align_log2) != 0) return -EIO;
    long off = ftell(fp); if (off < 0) return -EIO;
    // assign offsets then write bytes
    uint32_t cur = 0;
    for (lssize_t i = 0; i < spool.size; ++i) {
      spool.data[i].off = cur;
      const char* b = spool.data[i].bytes; lssize_t bl = spool.data[i].len;
      if (bl > 0) { if (fwrite(b, 1, (size_t)bl, fp) != (size_t)bl) return -EIO; }
      cur += (uint32_t)bl;
    }
    long end = ftell(fp); if (end < 0) return -EIO;
    string_entry.kind = (uint32_t)LSTI_SECT_STRING_BLOB;
    string_entry.file_off = (uint64_t)off;
    string_entry.size = (uint64_t)(end - off);
  }

  // Write SYMBOL_BLOB
  lsti_section_disk_t symbol_entry = {0};
  if (has_ypool) {
    if (fpad_to_align(fp, align_log2) != 0) return -EIO;
    long off = ftell(fp); if (off < 0) return -EIO;
    uint32_t cur = 0;
    for (lssize_t i = 0; i < ypool.size; ++i) {
      ypool.data[i].off = cur;
      const char* b = ypool.data[i].bytes; lssize_t bl = ypool.data[i].len;
      if (bl > 0) { if (fwrite(b, 1, (size_t)bl, fp) != (size_t)bl) return -EIO; }
      cur += (uint32_t)bl;
    }
    long end = ftell(fp); if (end < 0) return -EIO;
    symbol_entry.kind = (uint32_t)LSTI_SECT_SYMBOL_BLOB;
    symbol_entry.file_off = (uint64_t)off;
    symbol_entry.size = (uint64_t)(end - off);
  }

  // Write THUNK_TAB: layout = u32 count; u64 entry_off[count]; entries with headers and var part
  if (fpad_to_align(fp, align_log2) != 0) return -EIO;
  long thtab_off = ftell(fp); if (thtab_off < 0) return -EIO;
  uint32_t ncnt = (uint32_t)nodes.size;
  if (fwrite(&ncnt, 1, sizeof(ncnt), fp) != sizeof(ncnt)) return -EIO;
  long offs_pos = ftell(fp);
  if (offs_pos < 0) return -EIO;
  // reserve offset table
  for (uint32_t i = 0; i < ncnt; ++i) {
    uint64_t z = 0; if (fwrite(&z, 1, sizeof(z), fp) != sizeof(z)) return -EIO;
  }
  // write entries and backfill
  uint64_t* rel_offs = (uint64_t*)malloc(sizeof(uint64_t)*ncnt);
  if (!rel_offs) return -ENOMEM;
  for (uint32_t i = 0; i < ncnt; ++i) {
    long ent_off = ftell(fp); if (ent_off < 0) { free(rel_offs); return -EIO; }
    rel_offs[i] = (uint64_t)(ent_off - thtab_off);
    lsthunk_t* t = nodes.data[i];
    struct { uint8_t kind, flags; uint16_t pad; uint32_t aorl; uint32_t extra; } hdr2;
    memset(&hdr2, 0, sizeof(hdr2));
    // Precompute header fields
    enum { PAY_NONE, PAY_ALGE, PAY_BOTTOM } paykind = PAY_NONE;
    uint32_t pay_len_or_reserved = 0; // length field for ALGE constr or BOTTOM msg
    const uint32_t* child_ids = NULL; // not used directly; we write on the fly
    switch (lsthunk_get_type(t)) {
      case LSTTYPE_INT: {
        hdr2.kind = (uint8_t)LSTB_KIND_INT;
        const lsint_t* iv = lsthunk_get_int(t);
        hdr2.extra = (uint32_t)lsint_get(iv);
        break;
      }
      case LSTTYPE_STR: {
        hdr2.kind = (uint8_t)LSTB_KIND_STR;
        const lsstr_t* s = lsthunk_get_str(t);
        if (s) { int idx; ADD_SPOOL(lsstr_get_buf(s), lsstr_get_len(s), idx); hdr2.aorl = (uint32_t)lsstr_get_len(s); hdr2.extra = (uint32_t)spool.data[idx].off; }
        break;
      }
      case LSTTYPE_SYMBOL: {
        hdr2.kind = (uint8_t)LSTB_KIND_SYMBOL;
        const lsstr_t* s = lsthunk_get_symbol(t);
        if (s) { int idx; ADD_YPOOL(s, idx); hdr2.aorl = (uint32_t)lsstr_get_len(s); hdr2.extra = (uint32_t)ypool.data[idx].off; }
        break;
      }
      case LSTTYPE_ALGE: {
        hdr2.kind = (uint8_t)LSTB_KIND_ALGE; // numbering shared with LSTB
        const lsstr_t* c = lsthunk_get_constr(t);
        if (c) { int idx; ADD_YPOOL(c, idx); hdr2.extra = (uint32_t)ypool.data[idx].off; pay_len_or_reserved = (uint32_t)lsstr_get_len(c); }
        lssize_t ac = lsthunk_get_argc(t); hdr2.aorl = (uint32_t)ac; paykind = PAY_ALGE;
        break;
      }
      case LSTTYPE_BOTTOM: {
        hdr2.kind = (uint8_t)LSTB_KIND_BOTTOM;
        const char* msg = lsthunk_bottom_get_message(t);
        if (msg) { int idx; lssize_t mlen = (lssize_t)strlen(msg); ADD_SPOOL(msg, mlen, idx); hdr2.extra = (uint32_t)spool.data[idx].off; pay_len_or_reserved = (uint32_t)mlen; }
        lssize_t ac = lsthunk_bottom_get_argc(t); hdr2.aorl = (uint32_t)ac; paykind = PAY_BOTTOM;
        break;
      }
      default:
        free(rel_offs); return -ENOTSUP;
    }
    // Write header first
    if (fwrite(&hdr2, 1, sizeof(hdr2), fp) != sizeof(hdr2)) { free(rel_offs); return -EIO; }
    // Then write payload depending on kind
    if (paykind == PAY_ALGE) {
      // write constr len, then child ids
      if (fwrite(&pay_len_or_reserved, 1, sizeof(uint32_t), fp) != sizeof(uint32_t)) { free(rel_offs); return -EIO; }
      lssize_t ac = (lssize_t)hdr2.aorl;
      if (ac > 0) {
        lsthunk_t* const* as = lsthunk_get_args(t);
        for (lssize_t k = 0; k < ac; ++k) {
          int id; GET_ID(as[k], id); if (id < 0) { free(rel_offs); return -EIO; }
          uint32_t cid = (uint32_t)id; if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) { free(rel_offs); return -EIO; }
        }
      }
    } else if (paykind == PAY_BOTTOM) {
      // write message len, then related ids
      if (fwrite(&pay_len_or_reserved, 1, sizeof(uint32_t), fp) != sizeof(uint32_t)) { free(rel_offs); return -EIO; }
      lssize_t ac = (lssize_t)hdr2.aorl;
      if (ac > 0) {
        lsthunk_t* const* as = lsthunk_bottom_get_args(t);
        for (lssize_t k = 0; k < ac; ++k) {
          int id; GET_ID(as[k], id); if (id < 0) { free(rel_offs); return -EIO; }
          uint32_t cid = (uint32_t)id; if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) { free(rel_offs); return -EIO; }
        }
      }
    } else {
      // no payload
    }
  }
  long thtab_end = ftell(fp); if (thtab_end < 0) { free(rel_offs); return -EIO; }
  // backfill offset table
  if (fseek(fp, offs_pos, SEEK_SET) != 0) { free(rel_offs); return -EIO; }
  for (uint32_t i = 0; i < ncnt; ++i) {
    if (fwrite(&rel_offs[i], 1, sizeof(uint64_t), fp) != sizeof(uint64_t)) { free(rel_offs); return -EIO; }
  }
  if (fseek(fp, thtab_end, SEEK_SET) != 0) { free(rel_offs); return -EIO; }
  free(rel_offs);

  lsti_section_disk_t thtab_entry = {0};
  thtab_entry.kind = (uint32_t)LSTI_SECT_THUNK_TAB;
  thtab_entry.file_off = (uint64_t)thtab_off;
  thtab_entry.size = (uint64_t)(thtab_end - thtab_off);

  // Write ROOTS: u32 count + u32 ids
  if (fpad_to_align(fp, align_log2) != 0) return -EIO;
  long roots_off = ftell(fp); if (roots_off < 0) return -EIO;
  uint32_t count32 = (uint32_t)rootc;
  if (fwrite(&count32, 1, sizeof(count32), fp) != sizeof(count32)) return -EIO;
  for (lssize_t i = 0; i < rootc; ++i) {
  int id = -1; for (lssize_t j = 0; j < nodes.size; ++j) { if (nodes.data[j] == roots[i]) { id = (int)j; break; } }
  if (id < 0) return -EIO; uint32_t rid = (uint32_t)id;
    if (fwrite(&rid, 1, sizeof(rid), fp) != sizeof(rid)) return -EIO;
  }
  long roots_end = ftell(fp); if (roots_end < 0) return -EIO;
  lsti_section_disk_t roots_entry = {0};
  roots_entry.kind = (uint32_t)LSTI_SECT_ROOTS;
  roots_entry.file_off = (uint64_t)roots_off;
  roots_entry.size = (uint64_t)(roots_end - roots_off);

  // Backfill section table entries in order
  if (fseek(fp, sect_tbl_off, SEEK_SET) != 0) return -EIO;
  int idx = 0;
  if (has_spool) { if (fwrite(&string_entry, 1, sizeof(string_entry), fp) != sizeof(string_entry)) return -EIO; ++idx; }
  if (has_ypool) { if (fwrite(&symbol_entry, 1, sizeof(symbol_entry), fp) != sizeof(symbol_entry)) return -EIO; ++idx; }
  if (fwrite(&thtab_entry, 1, sizeof(thtab_entry), fp) != sizeof(thtab_entry)) return -EIO; ++idx;
  if (fwrite(&roots_entry, 1, sizeof(roots_entry), fp) != sizeof(roots_entry)) return -EIO; ++idx;
  // Seek to end
  if (fseek(fp, 0, SEEK_END) != 0) return -EIO;

  // free temp pools
  free(nodes.data); free(spool.data); free(ypool.data);
  return 0;
}

int lsti_map(const char* path, lsti_image_t* out_img) {
  if (!path || !out_img) return -EINVAL;
  FILE* fp = fopen(path, "rb");
  if (!fp) return -errno;
  if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -EIO; }
  long sz = ftell(fp);
  if (sz < 0) { fclose(fp); return -EIO; }
  if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return -EIO; }
  uint8_t* buf = (uint8_t*)malloc((size_t)sz);
  if (!buf) { fclose(fp); return -ENOMEM; }
  size_t rd = fread(buf, 1, (size_t)sz, fp);
  fclose(fp);
  if (rd != (size_t)sz) { free(buf); return -EIO; }
  out_img->base = buf;
  out_img->size = (lssize_t)sz;
  if ((lssize_t)sz >= (lssize_t)sizeof(lsti_header_disk_t)) {
    const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)buf;
    out_img->section_count = hdr->section_count;
    out_img->align_log2 = hdr->align_log2;
    out_img->flags = hdr->flags;
  } else {
    out_img->section_count = 0;
    out_img->align_log2 = 0;
    out_img->flags = 0u;
  }
  return 0;
}

int lsti_validate(const lsti_image_t* img) {
  if (!img || !img->base || img->size < (lssize_t)sizeof(lsti_header_disk_t)) return -EINVAL;
  const uint8_t* p = img->base;
  const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)p;
  if (hdr->magic != LSTI_MAGIC) return -EINVAL;
  if (hdr->version_major != LSTI_VERSION_MAJOR) return -EINVAL;
  if (hdr->align_log2 != (uint16_t)LSTI_ALIGN_8 && hdr->align_log2 != (uint16_t)LSTI_ALIGN_16) return -EINVAL;
  lssize_t need = (lssize_t)sizeof(lsti_header_disk_t) + (lssize_t)hdr->section_count * (lssize_t)sizeof(lsti_section_disk_t);
  if (img->size < need) return -EINVAL;
  const lsti_section_disk_t* sect = (const lsti_section_disk_t*)(p + sizeof(lsti_header_disk_t));
  // Gather section info and validate bounds/alignment
  uint64_t mask = ((uint64_t)1 << hdr->align_log2) - 1u;
  const lsti_section_disk_t *string_blob = NULL, *symbol_blob = NULL, *thunk_tab = NULL, *roots = NULL;
  for (uint16_t i = 0; i < hdr->section_count; ++i) {
    uint32_t kind = sect[i].kind;
    uint64_t off  = sect[i].file_off;
    uint64_t sz   = sect[i].size;
    if (sz > (uint64_t)img->size) return -EINVAL;
    if (off > (uint64_t)img->size) return -EINVAL;
    if (off + sz > (uint64_t)img->size) return -EINVAL;
    if ((off & mask) != 0) return -EINVAL;
    switch (kind) {
      case LSTI_SECT_STRING_BLOB: string_blob = &sect[i]; break;
      case LSTI_SECT_SYMBOL_BLOB: symbol_blob = &sect[i]; break;
      case LSTI_SECT_THUNK_TAB:   thunk_tab   = &sect[i]; break;
      case LSTI_SECT_ROOTS:       roots       = &sect[i]; break;
      default: break; // tolerate unknown sections for forward-compat
    }
  }
  // Minimal required sections
  if (!thunk_tab || !roots) return -EINVAL;
  // Validate THUNK_TAB structure: [u32 count][u64 offs[count]] followed by entries within section
  if (thunk_tab->size < sizeof(uint32_t)) return -EINVAL;
  const uint8_t* tt = img->base + (lssize_t)thunk_tab->file_off;
  uint32_t ncnt = 0; memcpy(&ncnt, tt, sizeof(uint32_t));
  uint64_t off_tbl_bytes = (uint64_t)ncnt * (uint64_t)sizeof(uint64_t);
  uint64_t header_bytes  = (uint64_t)sizeof(uint32_t) + off_tbl_bytes;
  if (thunk_tab->size < header_bytes) return -EINVAL;
  const uint64_t* offs = (const uint64_t*)(tt + sizeof(uint32_t));
  // Each entry offset must point within the THUNK_TAB section and be >= header_bytes
  for (uint32_t i = 0; i < ncnt; ++i) {
    uint64_t eo = offs[i];
    if (eo < header_bytes) return -EINVAL;
    if (eo >= thunk_tab->size) return -EINVAL;
    // For subset kinds, entry must at least contain our fixed header struct
    if (thunk_tab->size - eo < sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t)) return -EINVAL;
  }
  // Validate ROOTS: [u32 count][u32 ids...]
  if (roots->size < sizeof(uint32_t)) return -EINVAL;
  const uint8_t* rp = img->base + (lssize_t)roots->file_off;
  uint32_t rcnt = 0; memcpy(&rcnt, rp, sizeof(uint32_t));
  uint64_t roots_bytes = (uint64_t)sizeof(uint32_t) + (uint64_t)rcnt * (uint64_t)sizeof(uint32_t);
  if (roots->size < roots_bytes) return -EINVAL;
  const uint32_t* rids = (const uint32_t*)(rp + sizeof(uint32_t));
  for (uint32_t i = 0; i < rcnt; ++i) { if (rids[i] >= ncnt) return -EINVAL; }
  // Optional: for STR/SYMBOL, offsets must be within corresponding blobs when present
  if (string_blob || symbol_blob) {
    for (uint32_t i = 0; i < ncnt; ++i) {
      const uint8_t* ent = tt + offs[i];
      uint8_t kind = ent[0];
      uint32_t aorl = 0, extra = 0; memcpy(&aorl, ent + 4, sizeof(uint32_t)); memcpy(&extra, ent + 8, sizeof(uint32_t));
      if (kind == (uint8_t)LSTB_KIND_STR) {
        if (!string_blob) return -EINVAL;
        uint64_t sz = string_blob->size; uint64_t off = (uint64_t)extra, len = (uint64_t)aorl;
        if (off > sz || len > sz || off + len > sz) return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_SYMBOL) {
        if (!symbol_blob) return -EINVAL;
        uint64_t sz = symbol_blob->size; uint64_t off = (uint64_t)extra, len = (uint64_t)aorl;
        if (off > sz || len > sz || off + len > sz) return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_ALGE) {
        if (!symbol_blob) return -EINVAL;
        // payload starts after header: u32 constr_len, then child ids
        uint32_t constr_len = 0; memcpy(&constr_len, ent + sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t)+sizeof(uint32_t), sizeof(uint32_t));
        uint64_t sz = symbol_blob->size; uint64_t off = (uint64_t)extra, len = (uint64_t)constr_len;
        if (off > sz || len > sz || off + len > sz) return -EINVAL;
        // ensure payload fits: 4 + 4*argc
        uint64_t need = (uint64_t)sizeof(uint32_t) + (uint64_t)aorl * (uint64_t)sizeof(uint32_t);
        if (thunk_tab->size - offs[i] < (uint64_t)sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t)+sizeof(uint32_t) + need) return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_BOTTOM) {
        if (!string_blob) return -EINVAL;
        uint32_t msg_len = 0; memcpy(&msg_len, ent + sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t)+sizeof(uint32_t), sizeof(uint32_t));
        uint64_t sz = string_blob->size; uint64_t off = (uint64_t)extra, len = (uint64_t)msg_len;
        if (off > sz || len > sz || off + len > sz) return -EINVAL;
        uint64_t need = (uint64_t)sizeof(uint32_t) + (uint64_t)aorl * (uint64_t)sizeof(uint32_t);
        if (thunk_tab->size - offs[i] < (uint64_t)sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t)+sizeof(uint32_t) + need) return -EINVAL;
      }
    }
  }
  return 0;
}

int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc, struct lstenv* prelude_env) {
  (void)prelude_env;
  if (!img || !img->base || !out_roots || !out_rootc) return -EINVAL;
  if (lsti_validate(img) != 0) return -EINVAL;
  const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)img->base;
  const lsti_section_disk_t* sect = (const lsti_section_disk_t*)(img->base + sizeof(lsti_header_disk_t));
  const lsti_section_disk_t *thunk_tab = NULL, *roots = NULL, *string_blob = NULL, *symbol_blob = NULL;
  for (uint16_t i = 0; i < hdr->section_count; ++i) {
    if (sect[i].kind == (uint32_t)LSTI_SECT_THUNK_TAB) thunk_tab = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_ROOTS) roots = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_STRING_BLOB) string_blob = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_SYMBOL_BLOB) symbol_blob = &sect[i];
  }
  if (!thunk_tab || !roots) return -EINVAL;
  const uint8_t* tt = img->base + (lssize_t)thunk_tab->file_off;
  uint32_t ncnt = 0; memcpy(&ncnt, tt, sizeof(uint32_t));
  const uint64_t* offs = (const uint64_t*)(tt + sizeof(uint32_t));
  if (ncnt == 0) { *out_roots = NULL; *out_rootc = 0; return 0; }
  // Build an array of thunks; only INT is materialized for now; others return ENOSYS
  lsthunk_t** nodes = (lsthunk_t**)calloc(ncnt, sizeof(lsthunk_t*));
  if (!nodes) return -ENOMEM;
  typedef struct { lssize_t argc; uint32_t* ids; } pend_t;
  pend_t* pend = (pend_t*)calloc(ncnt, sizeof(pend_t));
  if (!pend) { free(nodes); return -ENOMEM; }
  const char* sbase = string_blob ? (const char*)(img->base + (lssize_t)string_blob->file_off) : NULL;
  const char* ybase = symbol_blob ? (const char*)(img->base + (lssize_t)symbol_blob->file_off) : NULL;
  for (uint32_t i = 0; i < ncnt; ++i) {
    const uint8_t* ent = tt + offs[i];
    uint8_t kind = ent[0];
    switch (kind) {
      case LSTB_KIND_INT: {
        uint32_t val = 0; memcpy(&val, ent + 8, sizeof(uint32_t));
        nodes[i] = lsthunk_new_int(lsint_new((int)val));
        break;
      }
      case LSTB_KIND_STR: {
        if (!string_blob) { free(nodes); return -EINVAL; }
        uint32_t len = 0, off = 0; memcpy(&len, ent + 4, sizeof(uint32_t)); memcpy(&off, ent + 8, sizeof(uint32_t));
        const char* base = (const char*)(img->base + (lssize_t)string_blob->file_off);
        const lsstr_t* s = lsstr_new(base + off, (lssize_t)len);
        nodes[i] = lsthunk_new_str(s);
        break;
      }
      case LSTB_KIND_SYMBOL: {
        if (!symbol_blob) { free(nodes); return -EINVAL; }
        uint32_t len = 0, off = 0; memcpy(&len, ent + 4, sizeof(uint32_t)); memcpy(&off, ent + 8, sizeof(uint32_t));
        const char* base = (const char*)(img->base + (lssize_t)symbol_blob->file_off);
        const lsstr_t* s = lsstr_new(base + off, (lssize_t)len);
        nodes[i] = lsthunk_new_symbol(s);
        break;
      }
      case LSTB_KIND_ALGE: {
        if (!symbol_blob) { free(nodes); free(pend); return -EINVAL; }
        // header fields
        uint32_t argc = 0, off = 0; memcpy(&argc, ent + 4, sizeof(uint32_t)); memcpy(&off, ent + 8, sizeof(uint32_t));
        const uint8_t* p = ent + 12; // payload starts after header
        uint32_t constr_len = 0; memcpy(&constr_len, p, sizeof(uint32_t)); p += 4;
        const lsstr_t* cstr = lsstr_new(ybase + off, (lssize_t)constr_len);
        // allocate via public API for two-phase wiring
        lsthunk_t* t = lsthunk_alloc_alge(cstr, (lssize_t)argc);
        nodes[i] = t;
        if (argc > 0) {
          pend[i].argc = (lssize_t)argc;
          pend[i].ids = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)argc);
          if (!pend[i].ids) { free(pend); free(nodes); return -ENOMEM; }
          for (uint32_t j = 0; j < argc; ++j) { uint32_t idj = 0; memcpy(&idj, p, sizeof(uint32_t)); p += 4; pend[i].ids[j] = idj; }
        }
        break;
      }
      case LSTB_KIND_BOTTOM: {
        if (!string_blob) { free(nodes); free(pend); return -EINVAL; }
        uint32_t rc = 0, off = 0; memcpy(&rc, ent + 4, sizeof(uint32_t)); memcpy(&off, ent + 8, sizeof(uint32_t));
        const uint8_t* p = ent + 12;
        uint32_t msg_len = 0; memcpy(&msg_len, p, sizeof(uint32_t)); p += 4;
        // copy message into a null-terminated C string
        char* m = (char*)lsmalloc((size_t)msg_len + 1);
        if (msg_len > 0) memcpy(m, sbase + off, (size_t)msg_len);
        m[msg_len] = '\0';
        // allocate with reserved related slots via API
        lsthunk_t* t = lsthunk_alloc_bottom(m, lstrace_take_pending_or_unknown(), (lssize_t)rc);
        nodes[i] = t;
        if (rc > 0) {
          pend[i].argc = (lssize_t)rc;
          pend[i].ids = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)rc);
          if (!pend[i].ids) { free(pend); free(nodes); return -ENOMEM; }
          for (uint32_t j = 0; j < rc; ++j) { uint32_t idj = 0; memcpy(&idj, p, sizeof(uint32_t)); p += 4; pend[i].ids[j] = idj; }
        }
        break;
      }
      default: {
        // Not supported in this subset
        for (uint32_t k = 0; k < i; ++k) { /* GC managed by Boehm */ }
        free(pend);
        free(nodes);
        return -ENOSYS;
      }
    }
  }
  // Second pass: wire pending edges
  for (uint32_t i = 0; i < ncnt; ++i) {
    if (pend[i].argc > 0 && pend[i].ids) {
      if (lsthunk_get_type(nodes[i]) == LSTTYPE_ALGE) {
        for (lssize_t j = 0; j < pend[i].argc; ++j) lsthunk_set_alge_arg(nodes[i], j, nodes[pend[i].ids[j]]);
      } else if (lsthunk_get_type(nodes[i]) == LSTTYPE_BOTTOM) {
        for (lssize_t j = 0; j < pend[i].argc; ++j) lsthunk_set_bottom_related(nodes[i], j, nodes[pend[i].ids[j]]);
      }
      free(pend[i].ids); pend[i].ids = NULL; pend[i].argc = 0;
    }
  }
  // Extract roots
  const uint8_t* rp = img->base + (lssize_t)roots->file_off;
  uint32_t rcnt = 0; memcpy(&rcnt, rp, sizeof(uint32_t));
  const uint32_t* rids = (const uint32_t*)(rp + sizeof(uint32_t));
  lsthunk_t** r = (lsthunk_t**)calloc(rcnt, sizeof(lsthunk_t*)); if (!r) { free(pend); free(nodes); return -ENOMEM; }
  for (uint32_t i = 0; i < rcnt; ++i) r[i] = nodes[rids[i]];
  *out_roots = r; *out_rootc = (lssize_t)rcnt;
  free(pend);
  free(nodes);
  return 0;
}

int lsti_unmap(lsti_image_t* img) {
  if (!img) return -EINVAL;
  free((void*)img->base);
  img->base = NULL;
  img->size = 0;
  return 0;
}

#endif

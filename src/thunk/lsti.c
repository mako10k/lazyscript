#include "lsti.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "thunk.h"

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

  // linear search for pools (ok for small graphs)
  auto find_pool = [](vec_pool_t* p, const char* bytes, lssize_t len) -> int {
    for (lssize_t i = 0; i < p->size; ++i) {
      if (p->data[i].len == len && memcmp(p->data[i].bytes, bytes, (size_t)len) == 0) return (int)i;
    }
    return -1;
  };

  // helper to add string to pool; return index
  auto add_spool = [&](const char* bytes, lssize_t len) -> int {
    if (!bytes || len < 0) return -1;
    int idx = find_pool(&spool, bytes, len);
    if (idx >= 0) return idx;
    pool_ent_t e; e.bytes = bytes; e.len = len; e.off = 0u;
    VEC_PUSH(spool, e, pool_ent_t);
    return (int)(spool.size - 1);
  };
  auto add_ypool = [&](const lsstr_t* s) -> int {
    if (!s) return -1;
    const char* buf = lsstr_get_buf(s);
    lssize_t len = lsstr_get_len(s);
    int idx = find_pool(&ypool, buf, len);
    if (idx >= 0) return idx;
    pool_ent_t e; e.bytes = buf; e.len = len; e.off = 0u;
    VEC_PUSH(ypool, e, pool_ent_t);
    return (int)(ypool.size - 1);
  };

  // naive ID lookup
  auto get_id = [&](lsthunk_t* t) -> int {
    for (lssize_t i = 0; i < nodes.size; ++i) if (nodes.data[i] == t) return (int)i;
    return -1;
  };
  // enqueue
  lssize_t qh = 0;
  for (lssize_t i = 0; i < rootc; ++i) {
    if (roots) { if (get_id(roots[i]) < 0) VEC_PUSH(nodes, roots[i], lsthunk_t*); }
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
        if (s) add_spool(lsstr_get_buf(s), lsstr_get_len(s));
        break;
      }
      case LSTTYPE_SYMBOL: {
        const lsstr_t* sym = lsthunk_get_symbol(t);
        if (sym) add_ypool(sym);
        break;
      }
      case LSTTYPE_ALGE: {
        const lsstr_t* c = lsthunk_get_constr(t);
        if (c) add_ypool(c);
        lssize_t ac = lsthunk_get_argc(t);
        lsthunk_t* const* as = lsthunk_get_args(t);
        for (lssize_t i = 0; i < ac; ++i) {
          lsthunk_t* ch = as[i];
          int id = get_id(ch); if (id < 0) { VEC_PUSH(nodes, ch, lsthunk_t*); id = (int)(nodes.size-1); }
          (void)id; // recorded later per-node
        }
        break;
      }
      case LSTTYPE_BOTTOM: {
        const char* msg = lsthunk_bottom_get_message(t);
        if (msg) add_spool(msg, (lssize_t)strlen(msg));
        lssize_t ac = lsthunk_bottom_get_argc(t);
        lsthunk_t* const* as = lsthunk_bottom_get_args(t);
        for (lssize_t i = 0; i < ac; ++i) {
          lsthunk_t* ch = as[i];
          int id = get_id(ch); if (id < 0) { VEC_PUSH(nodes, ch, lsthunk_t*); id = (int)(nodes.size-1); }
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
    switch (lsthunk_get_type(t)) {
      case LSTTYPE_INT: {
        hdr2.kind = 5; // LSTB/LSTI shared numbering assumption: INT=5
        const lsint_t* iv = lsthunk_get_int(t);
        hdr2.extra = (uint32_t)lsint_get(iv);
        break;
      }
      case LSTTYPE_STR: {
        hdr2.kind = 6; // STR
        const lsstr_t* s = lsthunk_get_str(t);
        if (s) {
          int idx = add_spool(lsstr_get_buf(s), lsstr_get_len(s));
          hdr2.aorl = (uint32_t)lsstr_get_len(s);
          hdr2.extra = (uint32_t)spool.data[idx].off;
        }
        break;
      }
      case LSTTYPE_SYMBOL: {
        hdr2.kind = 7; // SYMBOL
        const lsstr_t* s = lsthunk_get_symbol(t);
        if (s) {
          int idx = add_ypool(s);
          hdr2.aorl = (uint32_t)lsstr_get_len(s);
          hdr2.extra = (uint32_t)ypool.data[idx].off;
        }
        break;
      }
      case LSTTYPE_ALGE: {
        hdr2.kind = 0; // ALGE
        const lsstr_t* c = lsthunk_get_constr(t);
        if (c) { int idx = add_ypool(c); hdr2.extra = (uint32_t)ypool.data[idx].off; }
        lssize_t ac = lsthunk_get_argc(t); hdr2.aorl = (uint32_t)ac;
        if (ac > 0) {
          lsthunk_t* const* as = lsthunk_get_args(t);
          for (lssize_t k = 0; k < ac; ++k) {
            int id = get_id(as[k]); if (id < 0) { free(rel_offs); return -EIO; }
            uint32_t cid = (uint32_t)id; if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) { free(rel_offs); return -EIO; }
          }
        }
        break;
      }
      case LSTTYPE_BOTTOM: {
        hdr2.kind = 9; // BOTTOM
        const char* msg = lsthunk_bottom_get_message(t);
        if (msg) { int idx = add_spool(msg, (lssize_t)strlen(msg)); hdr2.extra = (uint32_t)spool.data[idx].off; }
        lssize_t ac = lsthunk_bottom_get_argc(t); hdr2.aorl = (uint32_t)ac;
        if (ac > 0) {
          lsthunk_t* const* as = lsthunk_bottom_get_args(t);
          for (lssize_t k = 0; k < ac; ++k) {
            int id = get_id(as[k]); if (id < 0) { free(rel_offs); return -EIO; }
            uint32_t cid = (uint32_t)id; if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) { free(rel_offs); return -EIO; }
          }
        }
        break;
      }
      default:
        free(rel_offs); return -ENOTSUP;
    }
    // finally write header at the beginning of entry: we wrote variable part first for ALGE/BOTTOM args; so move file to ent_off and write header then seek back after payload
    long cur = ftell(fp); if (cur < 0) { free(rel_offs); return -EIO; }
    if (fseek(fp, ent_off, SEEK_SET) != 0) { free(rel_offs); return -EIO; }
    if (fwrite(&hdr2, 1, sizeof(hdr2), fp) != sizeof(hdr2)) { free(rel_offs); return -EIO; }
    if (fseek(fp, cur, SEEK_SET) != 0) { free(rel_offs); return -EIO; }
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
    int id = get_id(roots[i]); if (id < 0) return -EIO; uint32_t rid = (uint32_t)id;
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
  for (uint16_t i = 0; i < hdr->section_count; ++i) {
    uint64_t off = sect[i].file_off;
    uint64_t sz  = sect[i].size;
    if (sz > (uint64_t)img->size) return -EINVAL;
    if (off > (uint64_t)img->size) return -EINVAL;
    if (off + sz > (uint64_t)img->size) return -EINVAL;
    (void)sect[i].kind; // unknown kinds are allowed; skip
  }
  return 0;
}

int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc, struct lstenv* prelude_env) {
  (void)img; (void)out_roots; (void)out_rootc; (void)prelude_env;
  // Not implemented yet
  return -ENOSYS;
}

int lsti_unmap(lsti_image_t* img) {
  if (!img) return -EINVAL;
  free((void*)img->base);
  img->base = NULL;
  img->size = 0;
  return 0;
}

#endif

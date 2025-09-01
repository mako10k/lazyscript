#include "lsti.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "thunk.h"
#include "common/malloc.h"
#include "runtime/trace.h"
#include "thunk_bin.h"
#include "common/ref.h"
#include <stddef.h>

#ifndef ENABLE_LSTI
// If not enabled, provide stubs that return ENOSYS to keep build/link stable.
int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc,
               const lsti_write_opts_t* opt) {
  (void)fp;
  (void)roots;
  (void)rootc;
  (void)opt;
  return -ENOSYS;
}
int lsti_map(const char* path, lsti_image_t* out_img) {
  (void)path;
  (void)out_img;
  return -ENOSYS;
}
int lsti_validate(const lsti_image_t* img) {
  (void)img;
  return -ENOSYS;
}
int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc,
                     struct lstenv* prelude_env) {
  (void)img;
  (void)out_roots;
  (void)out_rootc;
  (void)prelude_env;
  return -ENOSYS;
}
int lsti_unmap(lsti_image_t* img) {
  (void)img;
  return -ENOSYS;
}

#else

typedef struct lsti_header_disk {
  uint32_t magic;         // 'LSTI'
  uint16_t version_major; // =1
  uint16_t version_minor; // =0
  uint16_t section_count;
  uint16_t align_log2; // 3 or 4
  uint32_t flags;      // LSTI_F_*
} lsti_header_disk_t;

typedef struct lsti_section_disk {
  uint32_t kind;     // lsti_sect_kind_t
  uint32_t reserved; // 0
  uint64_t file_off; // absolute file offset
  uint64_t size;     // payload size in bytes
} lsti_section_disk_t;

static int fpad_to_align(FILE* fp, uint16_t align_log2) {
  long pos = ftell(fp);
  if (pos < 0)
    return -EIO;
  long                 mask      = (1L << align_log2) - 1L;
  long                 pad       = ((pos + mask) & ~mask) - pos;
  static const uint8_t zeros[16] = { 0 };
  while (pad > 0) {
    size_t chunk = (size_t)((pad > (long)sizeof(zeros)) ? sizeof(zeros) : pad);
    if (fwrite(zeros, 1, chunk, fp) != chunk)
      return -EIO;
    pad -= (long)chunk;
  }
  return 0;
}

int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc,
               const lsti_write_opts_t* opt) {
  if (!fp || rootc < 0)
    return -EINVAL;
  uint16_t align_log2 = opt ? opt->align_log2 : (uint16_t)LSTI_ALIGN_8;
  if (align_log2 != (uint16_t)LSTI_ALIGN_8 && align_log2 != (uint16_t)LSTI_ALIGN_16)
    return -EINVAL;
  if ((uint64_t)rootc > 0xFFFFFFFFu)
    return -EOVERFLOW;

  // -----------------
  // Build graph and pools: now support INT/STR/SYMBOL/ALGE/BOTTOM/APPL/CHOICE/REF and LAMBDA
  // -----------------
  typedef struct vec_th {
    lsthunk_t** data;
    lssize_t    size, cap;
  } vec_th_t;
  typedef struct vec_u32 {
    uint32_t* data;
    lssize_t  size, cap;
  } vec_u32_t;
  typedef struct vec_u8 {
    uint8_t* data;
    lssize_t size, cap;
  } vec_u8_t;
  typedef struct pool_ent {
    const char* bytes;
    lssize_t    len;
    uint32_t    off;
  } pool_ent_t;
  typedef struct vec_pool {
    pool_ent_t* data;
    lssize_t    size, cap;
  } vec_pool_t;
  // Pattern pool (flat unique list); we store pointers to thunk-side patterns for dedup during
  // write, but materializer will not reuse these pointers.
  typedef struct vec_pat {
    lstpat_t** data;
    lssize_t   size, cap;
  } vec_pat_t;
  vec_pat_t ppool = { 0 };

#define VEC_PUSH(vec, val, type)                                                                   \
  do {                                                                                             \
    if ((vec).size == (vec).cap) {                                                                 \
      lssize_t ncap = (vec).cap ? (vec).cap * 2 : 8;                                               \
      void*    np   = realloc((vec).data, (size_t)ncap * sizeof(type));                            \
      if (!np)                                                                                     \
        return -ENOMEM;                                                                            \
      (vec).data = (type*)np;                                                                      \
      (vec).cap  = ncap;                                                                           \
    }                                                                                              \
    (vec).data[(vec).size++] = (val);                                                              \
  } while (0)

  vec_th_t   nodes = { 0 };
  vec_u32_t  edges = { 0 }; // flattened child id list; we'll write per-node later
  vec_pool_t spool = { 0 }; // string blob: STR values and BOTTOM messages
  vec_pool_t ypool = { 0 }; // symbol blob: SYMBOL literals and ALGE constructors

  // add to string pool if not exists; return index
#define ADD_SPOOL(BYTES, LEN, OUT_IDX)                                                             \
  do {                                                                                             \
    const char* __b = (const char*)(BYTES);                                                        \
    lssize_t    __l = (LEN);                                                                       \
    int         __idx = -1;                                                                        \
    if (__b && __l >= 0) {                                                                         \
      for (lssize_t __i = 0; __i < spool.size; ++__i) {                                            \
        if (spool.data[__i].len == __l &&                                                          \
            ((__l == 0) || memcmp(spool.data[__i].bytes, __b, (size_t)__l) == 0)) {                \
          __idx = (int)__i;                                                                        \
          break;                                                                                   \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
    if (__idx < 0) {                                                                               \
      pool_ent_t __e;                                                                              \
      __e.bytes = __b;                                                                             \
      __e.len   = __l;                                                                             \
      __e.off   = 0u;                                                                              \
      VEC_PUSH(spool, __e, pool_ent_t);                                                            \
      __idx = (int)(spool.size - 1);                                                               \
    }                                                                                              \
    (OUT_IDX) = __idx;                                                                             \
  } while (0)

  // add to symbol pool if not exists; return index
#define ADD_YPOOL(LSSTR, OUT_IDX)                                                                  \
  do {                                                                                             \
    const lsstr_t* __s = (LSSTR);                                                                  \
    int            __idx = -1;                                                                     \
    if (__s) {                                                                                     \
      const char* __b = lsstr_get_buf(__s);                                                        \
      lssize_t    __l = lsstr_get_len(__s);                                                        \
      for (lssize_t __i = 0; __i < ypool.size; ++__i) {                                            \
        if (ypool.data[__i].len == __l &&                                                          \
            ((__l == 0) || memcmp(ypool.data[__i].bytes, __b, (size_t)__l) == 0)) {                \
          __idx = (int)__i;                                                                        \
          break;                                                                                   \
        }                                                                                          \
      }                                                                                            \
      if (__idx < 0) {                                                                             \
        pool_ent_t __e;                                                                            \
        __e.bytes = __b;                                                                           \
        __e.len   = __l;                                                                           \
        __e.off   = 0u;                                                                            \
        VEC_PUSH(ypool, __e, pool_ent_t);                                                          \
        __idx = (int)(ypool.size - 1);                                                             \
      }                                                                                            \
    }                                                                                              \
    (OUT_IDX) = __idx;                                                                             \
  } while (0)

  // find existing node id in nodes or -1
#define GET_ID(TH, OUT_ID)                                                                         \
  do {                                                                                             \
    int __found = -1;                                                                              \
    for (lssize_t __j = 0; __j < nodes.size; ++__j) {                                              \
      if (nodes.data[__j] == (TH)) {                                                               \
        __found = (int)__j;                                                                        \
        break;                                                                                     \
      }                                                                                            \
    }                                                                                              \
    (OUT_ID) = __found;                                                                            \
  } while (0)

  // enqueue roots (dedup)
  lssize_t qh = 0;
  for (lssize_t i = 0; i < rootc; ++i) {
    if (roots && roots[i]) {
      int __id;
      GET_ID(roots[i], __id);
      if (__id < 0)
        VEC_PUSH(nodes, roots[i], lsthunk_t*);
    }
  }
  // BFS to collect reachable thunks and fill pools
  while (qh < nodes.size) {
    lsthunk_t* t = nodes.data[qh++];
    switch (lsthunk_get_type(t)) {
    case LSTTYPE_INT: {
      // no pool entries
      break;
    }
    case LSTTYPE_STR: {
      const lsstr_t* s = lsthunk_get_str(t);
      if (s) {
        int __idx; ADD_SPOOL(lsstr_get_buf(s), lsstr_get_len(s), __idx); (void)__idx;
      }
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
        lsthunk_t* ch = as[i]; int id; GET_ID(ch, id); if (id < 0) VEC_PUSH(nodes, ch, lsthunk_t*);
      }
      break;
    }
    case LSTTYPE_APPL: {
      lsthunk_t* func = lsthunk_get_appl_func(t);
      if (func) { int id; GET_ID(func, id); if (id < 0) VEC_PUSH(nodes, func, lsthunk_t*); }
      lssize_t ac = lsthunk_get_argc(t);
      lsthunk_t* const* as = lsthunk_get_args(t);
      for (lssize_t i = 0; i < ac; ++i) {
        lsthunk_t* ch = as[i]; int id; GET_ID(ch, id); if (id < 0) VEC_PUSH(nodes, ch, lsthunk_t*);
      }
      break;
    }
    case LSTTYPE_LAMBDA: {
      // collect param pattern into pattern pool (dedup by pointer)
      lstpat_t* p = lsthunk_get_param(t);
      int pidx = -1;
      for (lssize_t i = 0; i < ppool.size; ++i) { if (ppool.data[i] == p) { pidx = (int)i; break; } }
      if (pidx < 0) {
        if (ppool.size == ppool.cap) {
          lssize_t ncap = ppool.cap ? ppool.cap * 2 : 8;
          void* np = realloc(ppool.data, (size_t)ncap * sizeof(lstpat_t*));
          if (!np) return -ENOMEM;
          ppool.data = (lstpat_t**)np; ppool.cap = ncap;
        }
        ppool.data[ppool.size++] = p;
      }
      lsthunk_t* b = lsthunk_get_body(t);
      if (b) { int id; GET_ID(b, id); if (id < 0) VEC_PUSH(nodes, b, lsthunk_t*); }
      break;
    }
    case LSTTYPE_CHOICE: {
      lsthunk_t* l = lsthunk_get_choice_left(t);
      lsthunk_t* r = lsthunk_get_choice_right(t);
      if (l) { int id; GET_ID(l, id); if (id < 0) VEC_PUSH(nodes, l, lsthunk_t*); }
      if (r) { int id; GET_ID(r, id); if (id < 0) VEC_PUSH(nodes, r, lsthunk_t*); }
      break;
    }
    case LSTTYPE_REF: {
      const lsstr_t* name = lsthunk_get_ref_name(t);
      if (name) { int __idx; ADD_YPOOL(name, __idx); (void)__idx; }
      break;
    }
    case LSTTYPE_BUILTIN: {
      const lsstr_t* bname = lsthunk_get_builtin_name(t);
      if (bname) { int __idx; ADD_YPOOL(bname, __idx); (void)__idx; }
      break;
    }
    case LSTTYPE_BOTTOM: {
      const char* msg = lsthunk_bottom_get_message(t);
      if (msg) { int __idx; ADD_SPOOL(msg, (lssize_t)strlen(msg), __idx); (void)__idx; }
      lssize_t ac = lsthunk_bottom_get_argc(t);
      lsthunk_t* const* as = lsthunk_bottom_get_args(t);
      for (lssize_t i = 0; i < ac; ++i) {
        lsthunk_t* ch = as[i]; int id; GET_ID(ch, id); if (id < 0) VEC_PUSH(nodes, ch, lsthunk_t*);
      }
      break;
    }
    default:
      // ignore unsupported kinds in writer (shouldn't happen)
      break;
    }
  }

  // -----------------
  // Write header + section table placeholders (we may include 2 optional pools)
  // -----------------
  int sect_count = 2; // THUNK_TAB + ROOTS
  int has_spool  = (spool.size > 0);
  int has_ypool  = (ypool.size > 0);
  int has_ppool  = (ppool.size > 0);
  if (has_spool)
    ++sect_count;
  if (has_ypool)
    ++sect_count;
  if (has_ppool)
    ++sect_count;
  // Header
  lsti_header_disk_t hdr;
  hdr.magic         = LSTI_MAGIC;
  hdr.version_major = LSTI_VERSION_MAJOR;
  hdr.version_minor = LSTI_VERSION_MINOR;
  hdr.section_count = (uint16_t)sect_count;
  hdr.align_log2    = align_log2;
  hdr.flags         = opt ? opt->flags : 0u;
  if (fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr))
    return -EIO;
  // Placeholder section table
  long sect_tbl_off = ftell(fp);
  if (sect_tbl_off < 0)
    return -EIO;
  for (int i = 0; i < sect_count; ++i) {
    lsti_section_disk_t zero = { 0 };
    if (fwrite(&zero, 1, sizeof(zero), fp) != sizeof(zero))
      return -EIO;
  }

  // Write STRING_BLOB
  lsti_section_disk_t string_entry = { 0 };
  if (has_spool) {
    if (fpad_to_align(fp, align_log2) != 0)
      return -EIO;
    long off = ftell(fp);
    if (off < 0)
      return -EIO;
    // Precompute offsets
    uint32_t cur = 0;
    for (lssize_t i = 0; i < spool.size; ++i) {
      spool.data[i].off = cur;
      cur += (uint32_t)spool.data[i].len;
    }
    // Write index header + index body: [u32 count][(u32 off,u32 len) * count]
    uint32_t index_bytes = (uint32_t)sizeof(uint32_t) + (uint32_t)spool.size * (uint32_t)(sizeof(uint32_t) * 2);
    if (fwrite(&index_bytes, 1, sizeof(index_bytes), fp) != sizeof(index_bytes))
      return -EIO;
    uint32_t scount = (uint32_t)spool.size;
    if (fwrite(&scount, 1, sizeof(scount), fp) != sizeof(scount))
      return -EIO;
    for (lssize_t i = 0; i < spool.size; ++i) {
      uint32_t o = spool.data[i].off;
      uint32_t l = (uint32_t)spool.data[i].len;
      if (fwrite(&o, 1, sizeof(o), fp) != sizeof(o) || fwrite(&l, 1, sizeof(l), fp) != sizeof(l))
        return -EIO;
    }
    // Write payload bytes in the same order
    for (lssize_t i = 0; i < spool.size; ++i) {
      const char* b = spool.data[i].bytes;
      lssize_t    bl = spool.data[i].len;
      if (bl > 0) {
        if (fwrite(b, 1, (size_t)bl, fp) != (size_t)bl)
          return -EIO;
      }
    }
    long end = ftell(fp);
    if (end < 0)
      return -EIO;
    string_entry.kind     = (uint32_t)LSTI_SECT_STRING_BLOB;
    string_entry.file_off = (uint64_t)off;
    string_entry.size     = (uint64_t)(end - off);
  }

  // Write SYMBOL_BLOB
  lsti_section_disk_t symbol_entry = { 0 };
  if (has_ypool) {
    if (fpad_to_align(fp, align_log2) != 0)
      return -EIO;
    long off = ftell(fp);
    if (off < 0)
      return -EIO;
    // Precompute offsets
    uint32_t cur = 0;
    for (lssize_t i = 0; i < ypool.size; ++i) {
      ypool.data[i].off = cur;
      cur += (uint32_t)ypool.data[i].len;
    }
    // Write index header + index body: [u32 count][(u32 off,u32 len) * count]
    uint32_t index_bytes = (uint32_t)sizeof(uint32_t) + (uint32_t)ypool.size * (uint32_t)(sizeof(uint32_t) * 2);
    if (fwrite(&index_bytes, 1, sizeof(index_bytes), fp) != sizeof(index_bytes))
      return -EIO;
    uint32_t ycount = (uint32_t)ypool.size;
    if (fwrite(&ycount, 1, sizeof(ycount), fp) != sizeof(ycount))
      return -EIO;
    for (lssize_t i = 0; i < ypool.size; ++i) {
      uint32_t o = ypool.data[i].off;
      uint32_t l = (uint32_t)ypool.data[i].len;
      if (fwrite(&o, 1, sizeof(o), fp) != sizeof(o) || fwrite(&l, 1, sizeof(l), fp) != sizeof(l))
        return -EIO;
    }
    // Write payload bytes
    for (lssize_t i = 0; i < ypool.size; ++i) {
      const char* b = ypool.data[i].bytes;
      lssize_t    bl = ypool.data[i].len;
      if (bl > 0) {
        if (fwrite(b, 1, (size_t)bl, fp) != (size_t)bl)
          return -EIO;
      }
    }
    long end = ftell(fp);
    if (end < 0)
      return -EIO;
    symbol_entry.kind     = (uint32_t)LSTI_SECT_SYMBOL_BLOB;
    symbol_entry.file_off = (uint64_t)off;
    symbol_entry.size     = (uint64_t)(end - off);
  }

  // Write PATTERN_TAB: [u32 count][u64 entry_off[count]] + entries
  lsti_section_disk_t ptab_entry = { 0 };
  if (has_ppool) {
    if (fpad_to_align(fp, align_log2) != 0)
      return -EIO;
    long ptab_off = ftell(fp);
    if (ptab_off < 0)
      return -EIO;
    // reserved index header for future use
    uint32_t ptab_index_bytes = 0;
    if (fwrite(&ptab_index_bytes, 1, sizeof(ptab_index_bytes), fp) != sizeof(ptab_index_bytes))
      return -EIO;
    uint32_t pcnt = (uint32_t)ppool.size;
    if (fwrite(&pcnt, 1, sizeof(pcnt), fp) != sizeof(pcnt))
      return -EIO;
    long poffs_pos = ftell(fp);
    if (poffs_pos < 0)
      return -EIO;
    for (uint32_t i = 0; i < pcnt; ++i) {
      uint64_t z = 0;
      if (fwrite(&z, 1, sizeof(z), fp) != sizeof(z))
        return -EIO;
    }
    uint64_t* poffs = (uint64_t*)malloc(sizeof(uint64_t) * pcnt);
    if (!poffs)
      return -ENOMEM;
    for (uint32_t i = 0; i < pcnt; ++i) {
      long ent_off = ftell(fp);
      if (ent_off < 0) {
        free(poffs);
        return -EIO;
      }
      poffs[i] = (uint64_t)(ent_off - ptab_off);
      const lstpat_t* p = ppool.data[i];
      // pat entry: u8 kind + payload (SYMBOL/STRING refs by len/offset; thunk ids not allowed)
      uint8_t kind = (uint8_t)lstpat_get_type(p);
      if (fwrite(&kind, 1, sizeof(kind), fp) != sizeof(kind)) {
        free(poffs);
        return -EIO;
      }
      switch (kind) {
      case LSPTYPE_ALGE: {
        const lsstr_t* c = lstpat_get_constr(p);
        int            yi;
        ADD_YPOOL(c, yi);
        uint32_t clen = (uint32_t)lsstr_get_len(c);
        uint32_t coff = (uint32_t)ypool.data[yi].off;
        uint32_t argc = (uint32_t)lstpat_get_argc(p);
        if (fwrite(&clen, 1, 4, fp) != 4 || fwrite(&coff, 1, 4, fp) != 4 ||
            fwrite(&argc, 1, 4, fp) != 4)
          return -EIO;
        lstpat_t* const* args = lstpat_get_args(p);
        for (uint32_t j = 0; j < argc; ++j) {
          // find child index in pool
          int cidx = -1;
          for (lssize_t k = 0; k < ppool.size; ++k)
            if (ppool.data[k] == args[j]) {
              cidx = (int)k;
              break;
            }
          if (cidx < 0)
            return -EIO; // children must be in pool already
          uint32_t cid = (uint32_t)cidx;
          if (fwrite(&cid, 1, 4, fp) != 4)
            return -EIO;
        }
        break;
      }
      case LSPTYPE_AS: {
        // payload: ref_pat_id, inner_pat_id (both pool indices)
        lstpat_t* pref = lstpat_get_ref(p);
        lstpat_t* pin  = lstpat_get_aspattern(p);
        int       idr = -1, idi = -1;
        for (lssize_t k = 0; k < ppool.size; ++k) {
          if (ppool.data[k] == pref)
            idr = (int)k;
          if (ppool.data[k] == pin)
            idi = (int)k;
        }
        if (idr < 0 || idi < 0)
          return -EIO;
        uint32_t r32 = (uint32_t)idr, i32 = (uint32_t)idi;
        if (fwrite(&r32, 1, 4, fp) != 4 || fwrite(&i32, 1, 4, fp) != 4)
          return -EIO;
        break;
      }
      case LSPTYPE_INT: {
        int64_t v = (int64_t)lsint_get(lstpat_get_int(p));
        if (fwrite(&v, 1, sizeof(v), fp) != sizeof(v))
          return -EIO;
        break;
      }
      case LSPTYPE_STR: {
        const lsstr_t* s = lstpat_get_str(p);
        int            si;
        ADD_SPOOL(lsstr_get_buf(s), lsstr_get_len(s), si);
        uint32_t slen = (uint32_t)lsstr_get_len(s);
        uint32_t soff = (uint32_t)spool.data[si].off;
        if (fwrite(&slen, 1, 4, fp) != 4 || fwrite(&soff, 1, 4, fp) != 4)
          return -EIO;
        break;
      }
      case LSPTYPE_REF: {
        // encode by symbol name using thunk-side accessor
        const lsstr_t* s = lstpat_get_refname(p);
        int            yi;
        ADD_YPOOL(s, yi);
        uint32_t ylen = (uint32_t)lsstr_get_len(s);
        uint32_t yoff = (uint32_t)ypool.data[yi].off;
        if (fwrite(&ylen, 1, 4, fp) != 4 || fwrite(&yoff, 1, 4, fp) != 4)
          return -EIO;
        break;
      }
      case LSPTYPE_WILDCARD: {
        // no payload
        break;
      }
      case LSPTYPE_OR: {
        // payload: left_id, right_id
        lstpat_t* l = lstpat_get_or_left(p);
        lstpat_t* r = lstpat_get_or_right(p);
        int       il = -1, ir = -1;
        for (lssize_t k = 0; k < ppool.size; ++k) {
          if (ppool.data[k] == l)
            il = (int)k;
          if (ppool.data[k] == r)
            ir = (int)k;
        }
        if (il < 0 || ir < 0)
          return -EIO;
        uint32_t l32 = (uint32_t)il, r32 = (uint32_t)ir;
        if (fwrite(&l32, 1, 4, fp) != 4 || fwrite(&r32, 1, 4, fp) != 4)
          return -EIO;
        break;
      }
      case LSPTYPE_CARET: {
        // payload: inner_id
        lstpat_t* in = lstpat_get_caret_inner(p);
        int       ii = -1;
        for (lssize_t k = 0; k < ppool.size; ++k)
          if (ppool.data[k] == in) {
            ii = (int)k;
            break;
          }
        if (ii < 0)
          return -EIO;
        uint32_t i32 = (uint32_t)ii;
        if (fwrite(&i32, 1, 4, fp) != 4)
          return -EIO;
        break;
      }
      default:
        free(poffs);
        return -ENOTSUP;
      }
    }
    long ptab_end = ftell(fp);
    if (ptab_end < 0) {
      free(poffs);
      return -EIO;
    }
    if (fseek(fp, poffs_pos, SEEK_SET) != 0) {
      free(poffs);
      return -EIO;
    }
    for (uint32_t i = 0; i < pcnt; ++i) {
      if (fwrite(&poffs[i], 1, sizeof(uint64_t), fp) != sizeof(uint64_t)) {
        free(poffs);
        return -EIO;
      }
    }
    if (fseek(fp, ptab_end, SEEK_SET) != 0) {
      free(poffs);
      return -EIO;
    }
    free(poffs);
    ptab_entry.kind     = (uint32_t)LSTI_SECT_PATTERN_TAB;
    ptab_entry.file_off = (uint64_t)ptab_off;
    ptab_entry.size     = (uint64_t)(ptab_end - ptab_off);
  }

  // Write THUNK_TAB: layout = u32 count; u64 entry_off[count]; entries with headers and var part
  if (fpad_to_align(fp, align_log2) != 0)
    return -EIO;
  long thtab_off = ftell(fp);
  if (thtab_off < 0)
    return -EIO;
  uint32_t ncnt = (uint32_t)nodes.size;
  if (fwrite(&ncnt, 1, sizeof(ncnt), fp) != sizeof(ncnt))
    return -EIO;
  long offs_pos = ftell(fp);
  if (offs_pos < 0)
    return -EIO;
  // reserve offset table
  for (uint32_t i = 0; i < ncnt; ++i) {
    uint64_t z = 0;
    if (fwrite(&z, 1, sizeof(z), fp) != sizeof(z))
      return -EIO;
  }
  // write entries and backfill
  uint64_t* rel_offs = (uint64_t*)malloc(sizeof(uint64_t) * ncnt);
  if (!rel_offs)
    return -ENOMEM;
  for (uint32_t i = 0; i < ncnt; ++i) {
    long ent_off = ftell(fp);
    if (ent_off < 0) {
      free(rel_offs);
      return -EIO;
    }
    rel_offs[i]  = (uint64_t)(ent_off - thtab_off);
    lsthunk_t* t = nodes.data[i];
    struct {
      uint8_t  kind, flags;
      uint16_t pad;
      uint32_t aorl;
      uint32_t extra;
    } hdr2;
    memset(&hdr2, 0, sizeof(hdr2));
    // Precompute header fields
    enum { PAY_NONE, PAY_ALGE, PAY_BOTTOM } paykind = PAY_NONE;
    uint32_t        pay_len_or_reserved = 0;    // length field for ALGE constr or BOTTOM msg
    const uint32_t* child_ids           = NULL; // not used directly; we write on the fly
    switch (lsthunk_get_type(t)) {
    case LSTTYPE_INT: {
      hdr2.kind         = (uint8_t)LSTB_KIND_INT;
      const lsint_t* iv = lsthunk_get_int(t);
      hdr2.extra        = (uint32_t)lsint_get(iv);
      break;
    }
    case LSTTYPE_STR: {
      hdr2.kind        = (uint8_t)LSTB_KIND_STR;
      const lsstr_t* s = lsthunk_get_str(t);
      if (s) {
        int idx;
        ADD_SPOOL(lsstr_get_buf(s), lsstr_get_len(s), idx);
        hdr2.aorl  = (uint32_t)lsstr_get_len(s);
        hdr2.extra = (uint32_t)spool.data[idx].off;
      }
      break;
    }
    case LSTTYPE_SYMBOL: {
      hdr2.kind        = (uint8_t)LSTB_KIND_SYMBOL;
      const lsstr_t* s = lsthunk_get_symbol(t);
      if (s) {
        int idx;
        ADD_YPOOL(s, idx);
        hdr2.aorl  = (uint32_t)lsstr_get_len(s);
        hdr2.extra = (uint32_t)ypool.data[idx].off;
      }
      break;
    }
    case LSTTYPE_APPL: {
      hdr2.kind = (uint8_t)LSTB_KIND_APPL;
      // header: aorl = argc, extra = func_id, payload: arg ids
      lsthunk_t* func = lsthunk_get_appl_func(t);
      int        fid  = -1;
      if (func) {
        GET_ID(func, fid);
      }
      if (fid < 0) {
        free(rel_offs);
        return -EIO;
      }
      lssize_t ac = lsthunk_get_argc(t);
      hdr2.aorl   = (uint32_t)ac;
      hdr2.extra  = (uint32_t)fid;
      break;
    }
    case LSTTYPE_CHOICE: {
      hdr2.kind = (uint8_t)LSTB_KIND_CHOICE;
      // aorl = choice_kind, payload: left_id, right_id
      hdr2.aorl = (uint32_t)lsthunk_get_choice_kind(t);
      hdr2.extra = 0;
      break;
    }
    case LSTTYPE_REF: {
      hdr2.kind         = (uint8_t)LSTB_KIND_REF;
      const lsstr_t* s  = lsthunk_get_ref_name(t);
      if (s) {
        int idx;
        ADD_YPOOL(s, idx);
        hdr2.aorl  = (uint32_t)lsstr_get_len(s);
        hdr2.extra = (uint32_t)ypool.data[idx].off;
      } else {
        free(rel_offs);
        return -EIO;
      }
      break;
    }
    case LSTTYPE_BUILTIN: {
      // Serialize builtin as REF to its name; arity/attrs are resolved via prelude env on load
      hdr2.kind        = (uint8_t)LSTB_KIND_REF;
      const lsstr_t* s = lsthunk_get_builtin_name(t);
      if (s) {
        int idx;
        ADD_YPOOL(s, idx);
        hdr2.aorl  = (uint32_t)lsstr_get_len(s);
        hdr2.extra = (uint32_t)ypool.data[idx].off;
      } else {
        free(rel_offs);
        return -EIO;
      }
      break;
    }
    case LSTTYPE_ALGE: {
      hdr2.kind        = (uint8_t)LSTB_KIND_ALGE; // numbering shared with LSTB
      const lsstr_t* c = lsthunk_get_constr(t);
      if (c) {
        int idx;
        ADD_YPOOL(c, idx);
        hdr2.extra          = (uint32_t)ypool.data[idx].off;
        pay_len_or_reserved = (uint32_t)lsstr_get_len(c);
      }
      lssize_t ac = lsthunk_get_argc(t);
      hdr2.aorl   = (uint32_t)ac;
      paykind     = PAY_ALGE;
      break;
    }
    case LSTTYPE_BOTTOM: {
      hdr2.kind       = (uint8_t)LSTB_KIND_BOTTOM;
      const char* msg = lsthunk_bottom_get_message(t);
      if (msg) {
        int      idx;
        lssize_t mlen = (lssize_t)strlen(msg);
        ADD_SPOOL(msg, mlen, idx);
        hdr2.extra          = (uint32_t)spool.data[idx].off;
        pay_len_or_reserved = (uint32_t)mlen;
      }
      lssize_t ac = lsthunk_bottom_get_argc(t);
      hdr2.aorl   = (uint32_t)ac;
      paykind     = PAY_BOTTOM;
      break;
    }
    case LSTTYPE_LAMBDA: {
      // header: aorl = pat_id (index in ppool), extra = body_id
      hdr2.kind = (uint8_t)LSTB_KIND_LAMBDA;
      // locate param pattern index
      lstpat_t* param = lsthunk_get_param(t);
      int       pid   = -1;
      for (lssize_t p = 0; p < ppool.size; ++p)
        if (ppool.data[p] == param) {
          pid = (int)p;
          break;
        }
      if (pid < 0) {
        free(rel_offs);
        return -EIO;
      }
      lsthunk_t* body = lsthunk_get_body(t);
      int        bid  = -1;
      if (body) {
        GET_ID(body, bid);
      }
      if (bid < 0) {
        free(rel_offs);
        return -EIO;
      }
      hdr2.aorl  = (uint32_t)pid;
      hdr2.extra = (uint32_t)bid;
      break;
    }
    default:
      free(rel_offs);
      return -ENOTSUP;
    }
    // Write header first
    if (fwrite(&hdr2, 1, sizeof(hdr2), fp) != sizeof(hdr2)) {
      free(rel_offs);
      return -EIO;
    }
    // Then write payload depending on kind
    if (paykind == PAY_ALGE) {
      // write constr len, then child ids
      if (fwrite(&pay_len_or_reserved, 1, sizeof(uint32_t), fp) != sizeof(uint32_t)) {
        free(rel_offs);
        return -EIO;
      }
      lssize_t ac = (lssize_t)hdr2.aorl;
      if (ac > 0) {
        lsthunk_t* const* as = lsthunk_get_args(t);
        for (lssize_t k = 0; k < ac; ++k) {
          int id;
          GET_ID(as[k], id);
          if (id < 0) {
            free(rel_offs);
            return -EIO;
          }
          uint32_t cid = (uint32_t)id;
          if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) {
            free(rel_offs);
            return -EIO;
          }
        }
      }
  } else if (paykind == PAY_BOTTOM) {
      // write message len, then related ids
      if (fwrite(&pay_len_or_reserved, 1, sizeof(uint32_t), fp) != sizeof(uint32_t)) {
        free(rel_offs);
        return -EIO;
      }
      lssize_t ac = (lssize_t)hdr2.aorl;
      if (ac > 0) {
        lsthunk_t* const* as = lsthunk_bottom_get_args(t);
        for (lssize_t k = 0; k < ac; ++k) {
          int id;
          GET_ID(as[k], id);
          if (id < 0) {
            free(rel_offs);
            return -EIO;
          }
          uint32_t cid = (uint32_t)id;
          if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) {
            free(rel_offs);
            return -EIO;
          }
        }
      }
    } else {
      // If APPL: payload is arg ids; if CHOICE: payload is 2 ids; if REF/INT/STR/SYMBOL: none
      if (lsthunk_get_type(t) == LSTTYPE_APPL) {
        lssize_t          ac = (lssize_t)hdr2.aorl;
        lsthunk_t* const* as = lsthunk_get_args(t);
        for (lssize_t k = 0; k < ac; ++k) {
          int id;
          GET_ID(as[k], id);
          if (id < 0) {
            free(rel_offs);
            return -EIO;
          }
          uint32_t cid = (uint32_t)id;
          if (fwrite(&cid, 1, sizeof(cid), fp) != sizeof(cid)) {
            free(rel_offs);
            return -EIO;
          }
        }
      } else if (lsthunk_get_type(t) == LSTTYPE_CHOICE) {
        lsthunk_t* l = lsthunk_get_choice_left(t);
        lsthunk_t* r = lsthunk_get_choice_right(t);
        int        lid = -1, rid = -1;
        if (l) GET_ID(l, lid);
        if (r) GET_ID(r, rid);
        if (lid < 0 || rid < 0) {
          free(rel_offs);
          return -EIO;
        }
        uint32_t l32 = (uint32_t)lid, r32 = (uint32_t)rid;
        if (fwrite(&l32, 1, sizeof(l32), fp) != sizeof(l32) ||
            fwrite(&r32, 1, sizeof(r32), fp) != sizeof(r32)) {
          free(rel_offs);
          return -EIO;
        }
      } else {
        // no payload
      }
    }
  }
  long thtab_end = ftell(fp);
  if (thtab_end < 0) {
    free(rel_offs);
    return -EIO;
  }
  // backfill offset table
  if (fseek(fp, offs_pos, SEEK_SET) != 0) {
    free(rel_offs);
    return -EIO;
  }
  for (uint32_t i = 0; i < ncnt; ++i) {
    if (fwrite(&rel_offs[i], 1, sizeof(uint64_t), fp) != sizeof(uint64_t)) {
      free(rel_offs);
      return -EIO;
    }
  }
  if (fseek(fp, thtab_end, SEEK_SET) != 0) {
    free(rel_offs);
    return -EIO;
  }
  free(rel_offs);

  lsti_section_disk_t thtab_entry = { 0 };
  thtab_entry.kind                = (uint32_t)LSTI_SECT_THUNK_TAB;
  thtab_entry.file_off            = (uint64_t)thtab_off;
  thtab_entry.size                = (uint64_t)(thtab_end - thtab_off);

  // Write ROOTS: u32 count + u32 ids
  if (fpad_to_align(fp, align_log2) != 0)
    return -EIO;
  long roots_off = ftell(fp);
  if (roots_off < 0)
    return -EIO;
  uint32_t count32 = (uint32_t)rootc;
  if (fwrite(&count32, 1, sizeof(count32), fp) != sizeof(count32))
    return -EIO;
  for (lssize_t i = 0; i < rootc; ++i) {
    int id = -1;
    for (lssize_t j = 0; j < nodes.size; ++j) {
      if (nodes.data[j] == roots[i]) {
        id = (int)j;
        break;
      }
    }
    if (id < 0)
      return -EIO;
    uint32_t rid = (uint32_t)id;
    if (fwrite(&rid, 1, sizeof(rid), fp) != sizeof(rid))
      return -EIO;
  }
  long roots_end = ftell(fp);
  if (roots_end < 0)
    return -EIO;
  lsti_section_disk_t roots_entry = { 0 };
  roots_entry.kind                = (uint32_t)LSTI_SECT_ROOTS;
  roots_entry.file_off            = (uint64_t)roots_off;
  roots_entry.size                = (uint64_t)(roots_end - roots_off);

  // Backfill section table entries in order
  if (fseek(fp, sect_tbl_off, SEEK_SET) != 0)
    return -EIO;
  int idx = 0;
  if (has_spool) {
    if (fwrite(&string_entry, 1, sizeof(string_entry), fp) != sizeof(string_entry))
      return -EIO;
    ++idx;
  }
  if (has_ypool) {
    if (fwrite(&symbol_entry, 1, sizeof(symbol_entry), fp) != sizeof(symbol_entry))
      return -EIO;
    ++idx;
  }
  if (has_ppool) {
    if (fwrite(&ptab_entry, 1, sizeof(ptab_entry), fp) != sizeof(ptab_entry))
      return -EIO;
    ++idx;
  }
  if (fwrite(&thtab_entry, 1, sizeof(thtab_entry), fp) != sizeof(thtab_entry))
    return -EIO;
  ++idx;
  if (fwrite(&roots_entry, 1, sizeof(roots_entry), fp) != sizeof(roots_entry))
    return -EIO;
  ++idx;
  // Seek to end
  if (fseek(fp, 0, SEEK_END) != 0)
    return -EIO;

  // free temp pools
  free(nodes.data);
  free(spool.data);
  free(ypool.data);
  free(ppool.data);
  return 0;
}

int lsti_map(const char* path, lsti_image_t* out_img) {
  if (!path || !out_img)
    return -EINVAL;
  FILE* fp = fopen(path, "rb");
  if (!fp)
    return -errno;
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return -EIO;
  }
  long sz = ftell(fp);
  if (sz < 0) {
    fclose(fp);
    return -EIO;
  }
  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    return -EIO;
  }
  uint8_t* buf = (uint8_t*)malloc((size_t)sz);
  if (!buf) {
    fclose(fp);
    return -ENOMEM;
  }
  size_t rd = fread(buf, 1, (size_t)sz, fp);
  fclose(fp);
  if (rd != (size_t)sz) {
    free(buf);
    return -EIO;
  }
  out_img->base = buf;
  out_img->size = (lssize_t)sz;
  if ((lssize_t)sz >= (lssize_t)sizeof(lsti_header_disk_t)) {
    const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)buf;
    out_img->section_count        = hdr->section_count;
    out_img->align_log2           = hdr->align_log2;
    out_img->flags                = hdr->flags;
  } else {
    out_img->section_count = 0;
    out_img->align_log2    = 0;
    out_img->flags         = 0u;
  }
  return 0;
}

int lsti_validate(const lsti_image_t* img) {
  if (!img || !img->base || img->size < (lssize_t)sizeof(lsti_header_disk_t))
    return -EINVAL;
  const uint8_t*            p   = img->base;
  const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)p;
  if (hdr->magic != LSTI_MAGIC)
    return -EINVAL;
  if (hdr->version_major != LSTI_VERSION_MAJOR)
    return -EINVAL;
  if (hdr->align_log2 != (uint16_t)LSTI_ALIGN_8 && hdr->align_log2 != (uint16_t)LSTI_ALIGN_16)
    return -EINVAL;
  lssize_t need = (lssize_t)sizeof(lsti_header_disk_t) +
                  (lssize_t)hdr->section_count * (lssize_t)sizeof(lsti_section_disk_t);
  if (img->size < need)
    return -EINVAL;
  const lsti_section_disk_t* sect = (const lsti_section_disk_t*)(p + sizeof(lsti_header_disk_t));
  // Gather section info and validate bounds/alignment
  uint64_t                   mask        = ((uint64_t)1 << hdr->align_log2) - 1u;
  const lsti_section_disk_t *string_blob = NULL, *symbol_blob = NULL, *pattern_tab = NULL,
                            *thunk_tab = NULL, *roots = NULL;
  for (uint16_t i = 0; i < hdr->section_count; ++i) {
    uint32_t kind = sect[i].kind;
    uint64_t off  = sect[i].file_off;
    uint64_t sz   = sect[i].size;
    if (sz > (uint64_t)img->size)
      return -EINVAL;
    if (off > (uint64_t)img->size)
      return -EINVAL;
    if (off + sz > (uint64_t)img->size)
      return -EINVAL;
    if ((off & mask) != 0)
      return -EINVAL;
    switch (kind) {
    case LSTI_SECT_STRING_BLOB:
      string_blob = &sect[i];
      break;
    case LSTI_SECT_SYMBOL_BLOB:
      symbol_blob = &sect[i];
      break;
    case LSTI_SECT_THUNK_TAB:
      thunk_tab = &sect[i];
      break;
    case LSTI_SECT_PATTERN_TAB:
      pattern_tab = &sect[i];
      break;
    case LSTI_SECT_ROOTS:
      roots = &sect[i];
      break;
    default:
      break; // tolerate unknown sections for forward-compat
    }
  }
  // Minimal required sections
  if (!thunk_tab || !roots)
    return -EINVAL;
  // Validate THUNK_TAB structure: [u32 count][u64 offs[count]] followed by entries within section
  if (thunk_tab->size < sizeof(uint32_t))
    return -EINVAL;
  const uint8_t* tt   = img->base + (lssize_t)thunk_tab->file_off;
  uint32_t       ncnt = 0;
  memcpy(&ncnt, tt, sizeof(uint32_t));
  uint64_t off_tbl_bytes = (uint64_t)ncnt * (uint64_t)sizeof(uint64_t);
  uint64_t header_bytes  = (uint64_t)sizeof(uint32_t) + off_tbl_bytes;
  if (thunk_tab->size < header_bytes)
    return -EINVAL;
  const uint64_t* offs = (const uint64_t*)(tt + sizeof(uint32_t));
  // Each entry offset must point within the THUNK_TAB section and be >= header_bytes
  for (uint32_t i = 0; i < ncnt; ++i) {
    uint64_t eo = offs[i];
    if (eo < header_bytes)
      return -EINVAL;
    if (eo >= thunk_tab->size)
      return -EINVAL;
    // For subset kinds, entry must at least contain our fixed header struct
    if (thunk_tab->size - eo <
        sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t))
      return -EINVAL;
  }
  // Validate ROOTS: [u32 count][u32 ids...]
  if (roots->size < sizeof(uint32_t))
    return -EINVAL;
  const uint8_t* rp   = img->base + (lssize_t)roots->file_off;
  uint32_t       rcnt = 0;
  memcpy(&rcnt, rp, sizeof(uint32_t));
  uint64_t roots_bytes = (uint64_t)sizeof(uint32_t) + (uint64_t)rcnt * (uint64_t)sizeof(uint32_t);
  if (roots->size < roots_bytes)
    return -EINVAL;
  const uint32_t* rids = (const uint32_t*)(rp + sizeof(uint32_t));
  for (uint32_t i = 0; i < rcnt; ++i) {
    if (rids[i] >= ncnt)
      return -EINVAL;
  }
  // Read reserved index headers for blobs/tables
  uint32_t string_index_bytes = 0, symbol_index_bytes = 0, pattern_index_bytes = 0;
  const uint8_t* string_payload_base = NULL;
  const uint8_t* symbol_payload_base = NULL;
  const uint8_t* pattern_payload_base = NULL;
  if (string_blob) {
    if (string_blob->size < sizeof(uint32_t)) return -EINVAL;
    memcpy(&string_index_bytes, img->base + (lssize_t)string_blob->file_off, sizeof(uint32_t));
    if (string_blob->size < (uint64_t)sizeof(uint32_t) + string_index_bytes) return -EINVAL;
    string_payload_base = img->base + (lssize_t)string_blob->file_off + sizeof(uint32_t) + string_index_bytes;
    // If index present: [u32 count][(u32 off,u32 len) * count]
    if (string_index_bytes) {
      if (string_index_bytes < sizeof(uint32_t)) return -EINVAL;
      const uint8_t* ip = img->base + (lssize_t)string_blob->file_off + sizeof(uint32_t);
      uint32_t scnt = 0; memcpy(&scnt, ip, sizeof(uint32_t));
      uint64_t need = (uint64_t)sizeof(uint32_t) + (uint64_t)scnt * (uint64_t)(sizeof(uint32_t) * 2);
      if ((uint64_t)string_index_bytes < need) return -EINVAL;
      uint64_t payload_sz = string_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)string_index_bytes;
      const uint8_t* p = ip + sizeof(uint32_t);
      uint64_t prev_end = 0;
      for (uint32_t i = 0; i < scnt; ++i) {
        uint32_t o = 0, l = 0;
        memcpy(&o, p, sizeof(uint32_t));
        memcpy(&l, p + 4, sizeof(uint32_t));
        p += 8;
        uint64_t off = (uint64_t)o, len = (uint64_t)l;
        if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
        if (off < prev_end) // overlap or disorder
          return -EINVAL;
        prev_end = off + len;
      }
    }
  }
  if (symbol_blob) {
    if (symbol_blob->size < sizeof(uint32_t)) return -EINVAL;
    memcpy(&symbol_index_bytes, img->base + (lssize_t)symbol_blob->file_off, sizeof(uint32_t));
    if (symbol_blob->size < (uint64_t)sizeof(uint32_t) + symbol_index_bytes) return -EINVAL;
    symbol_payload_base = img->base + (lssize_t)symbol_blob->file_off + sizeof(uint32_t) + symbol_index_bytes;
    if (symbol_index_bytes) {
      if (symbol_index_bytes < sizeof(uint32_t)) return -EINVAL;
      const uint8_t* ip = img->base + (lssize_t)symbol_blob->file_off + sizeof(uint32_t);
      uint32_t ycnt = 0; memcpy(&ycnt, ip, sizeof(uint32_t));
      uint64_t need = (uint64_t)sizeof(uint32_t) + (uint64_t)ycnt * (uint64_t)(sizeof(uint32_t) * 2);
      if ((uint64_t)symbol_index_bytes < need) return -EINVAL;
      uint64_t payload_sz = symbol_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)symbol_index_bytes;
      const uint8_t* p = ip + sizeof(uint32_t);
      uint64_t prev_end = 0;
      for (uint32_t i = 0; i < ycnt; ++i) {
        uint32_t o = 0, l = 0;
        memcpy(&o, p, sizeof(uint32_t));
        memcpy(&l, p + 4, sizeof(uint32_t));
        p += 8;
        uint64_t off = (uint64_t)o, len = (uint64_t)l;
        if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
        if (off < prev_end)
          return -EINVAL;
        prev_end = off + len;
      }
    }
  }
  if (pattern_tab) {
    if (pattern_tab->size < sizeof(uint32_t) + sizeof(uint32_t)) return -EINVAL; // index_bytes + count
    memcpy(&pattern_index_bytes, img->base + (lssize_t)pattern_tab->file_off, sizeof(uint32_t));
    if (pattern_tab->size < (uint64_t)sizeof(uint32_t) + pattern_index_bytes + sizeof(uint32_t)) return -EINVAL;
    pattern_payload_base = img->base + (lssize_t)pattern_tab->file_off + sizeof(uint32_t) + pattern_index_bytes;
  }
  // Optional: for STR/SYMBOL, offsets must be within corresponding blobs when present
  if (string_blob || symbol_blob) {
    for (uint32_t i = 0; i < ncnt; ++i) {
      const uint8_t* ent  = tt + offs[i];
      uint8_t        kind = ent[0];
      uint32_t       aorl = 0, extra = 0;
      memcpy(&aorl, ent + 4, sizeof(uint32_t));
      memcpy(&extra, ent + 8, sizeof(uint32_t));
      if (kind == (uint8_t)LSTB_KIND_STR) {
        if (!string_blob)
          return -EINVAL;
        // payload size excludes the 4B header and index
        uint64_t payload_sz = string_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)string_index_bytes;
        uint64_t off = (uint64_t)extra, len = (uint64_t)aorl;
        if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_SYMBOL) {
        if (!symbol_blob)
          return -EINVAL;
        uint64_t payload_sz = symbol_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)symbol_index_bytes;
        uint64_t off = (uint64_t)extra, len = (uint64_t)aorl;
        if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
  } else if (kind == (uint8_t)LSTB_KIND_ALGE) {
        if (!symbol_blob)
          return -EINVAL;
        // payload starts after header: u32 constr_len, then child ids
        uint32_t constr_len = 0;
        memcpy(&constr_len,
               ent + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) +
                   sizeof(uint32_t),
               sizeof(uint32_t));
  uint64_t payload_sz = symbol_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)symbol_index_bytes;
  uint64_t off = (uint64_t)extra, len = (uint64_t)constr_len;
  if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
        // ensure payload fits: 4 + 4*argc
        uint64_t need = (uint64_t)sizeof(uint32_t) + (uint64_t)aorl * (uint64_t)sizeof(uint32_t);
        if (thunk_tab->size - offs[i] < (uint64_t)sizeof(uint8_t) + sizeof(uint8_t) +
                                            sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                                            need)
          return -EINVAL;
  } else if (kind == (uint8_t)LSTB_KIND_BOTTOM) {
  if (!string_blob)
          return -EINVAL;
        uint32_t msg_len = 0;
        memcpy(&msg_len,
               ent + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) +
                   sizeof(uint32_t),
               sizeof(uint32_t));
  uint64_t payload_sz = string_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)string_index_bytes;
  uint64_t off = (uint64_t)extra, len = (uint64_t)msg_len;
  if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
        uint64_t need = (uint64_t)sizeof(uint32_t) + (uint64_t)aorl * (uint64_t)sizeof(uint32_t);
        if (thunk_tab->size - offs[i] < (uint64_t)sizeof(uint8_t) + sizeof(uint8_t) +
                                            sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                                            need)
          return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_APPL) {
        // payload: arg ids[argc]
        uint64_t need = (uint64_t)aorl * (uint64_t)sizeof(uint32_t);
        uint64_t have = thunk_tab->size - offs[i];
        if (have < (uint64_t)sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) +
                           sizeof(uint32_t) + sizeof(uint32_t) + need)
          return -EINVAL;
        if (extra >= ncnt)
          return -EINVAL; // func_id
      } else if (kind == (uint8_t)LSTB_KIND_CHOICE) {
        // aorl=choice_kind in 1..3; payload: left,right
        if (aorl < 1 || aorl > 3)
          return -EINVAL;
        uint64_t need = 2u * (uint64_t)sizeof(uint32_t);
        uint64_t have = thunk_tab->size - offs[i];
        if (have < (uint64_t)sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) +
                           sizeof(uint32_t) + sizeof(uint32_t) + need)
          return -EINVAL;
        // we'll check ids range in materializer, but bounds here too
        const uint8_t* p = ent + 12;
        uint32_t       lid = 0, rid = 0;
        memcpy(&lid, p, sizeof(uint32_t));
        memcpy(&rid, p + 4, sizeof(uint32_t));
        if (lid >= ncnt || rid >= ncnt)
          return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_REF) {
        if (!symbol_blob)
          return -EINVAL;
        uint64_t payload_sz = symbol_blob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)symbol_index_bytes;
        uint64_t off = (uint64_t)extra, len = (uint64_t)aorl;
        if (off > payload_sz || len > payload_sz || off + len > payload_sz)
          return -EINVAL;
      } else if (kind == (uint8_t)LSTB_KIND_LAMBDA) {
        // header stores indices: aorl=pat_id, extra=body_id; ensure pat_id < pcount and body_id < ncnt
        if (!pattern_tab)
          return -EINVAL;
        uint32_t pid = 0, bid = 0;
        memcpy(&pid, ent + 4, sizeof(uint32_t));
        memcpy(&bid, ent + 8, sizeof(uint32_t));
        // pattern_tab structure: [u32 index_bytes][u32 count][u64 offs[count]]
        if (pattern_tab->size < sizeof(uint32_t) + sizeof(uint32_t))
          return -EINVAL;
        uint32_t pcnt = 0;
        memcpy(&pcnt, pattern_payload_base, sizeof(uint32_t));
        if (pid >= pcnt || bid >= ncnt)
          return -EINVAL;
      }
    }
  }
  return 0;
}

int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc,
                     struct lstenv* prelude_env) {
  (void)prelude_env;
  if (!img || !img->base || !out_roots || !out_rootc)
    return -EINVAL;
  if (lsti_validate(img) != 0)
    return -EINVAL;
  const lsti_header_disk_t*  hdr = (const lsti_header_disk_t*)img->base;
  const lsti_section_disk_t* sect =
      (const lsti_section_disk_t*)(img->base + sizeof(lsti_header_disk_t));
  const lsti_section_disk_t *thunk_tab = NULL, *roots = NULL, *string_blob = NULL,
                            *symbol_blob = NULL, *pattern_tab = NULL;
  for (uint16_t i = 0; i < hdr->section_count; ++i) {
    if (sect[i].kind == (uint32_t)LSTI_SECT_THUNK_TAB)
      thunk_tab = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_ROOTS)
      roots = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_STRING_BLOB)
      string_blob = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_SYMBOL_BLOB)
      symbol_blob = &sect[i];
    else if (sect[i].kind == (uint32_t)LSTI_SECT_PATTERN_TAB)
      pattern_tab = &sect[i];
  }
  if (!thunk_tab || !roots)
    return -EINVAL;
  const uint8_t* tt   = img->base + (lssize_t)thunk_tab->file_off;
  uint32_t       ncnt = 0;
  memcpy(&ncnt, tt, sizeof(uint32_t));
  const uint64_t* offs = (const uint64_t*)(tt + sizeof(uint32_t));
  if (ncnt == 0) {
    *out_roots = NULL;
    *out_rootc = 0;
    return 0;
  }
  // Build an array of thunks; only INT is materialized for now; others return ENOSYS
  lsthunk_t** nodes = (lsthunk_t**)calloc(ncnt, sizeof(lsthunk_t*));
  if (!nodes)
    return -ENOMEM;
  typedef struct {
    uint8_t   kind;   // 0=none,1=ALGE,2=BOTTOM,3=APPL,4=CHOICE
    lssize_t  argc;   // ALGE/BOTTOM/APP: number of IDs in ids[]; CHOICE: 2
    uint32_t* ids;    // children ids (APP=args; CHOICE=[left,right])
    uint32_t  extra;  // APP=function id
  } pend_t;
  pend_t* pend = (pend_t*)calloc(ncnt, sizeof(pend_t));
  if (!pend) {
    free(nodes);
    return -ENOMEM;
  }
  // blob payload bases after index headers
  const char* sbase = NULL;
  const char* ybase = NULL;
  if (string_blob) {
    uint32_t ib = 0; memcpy(&ib, img->base + (lssize_t)string_blob->file_off, sizeof(uint32_t));
    sbase = (const char*)(img->base + (lssize_t)string_blob->file_off + sizeof(uint32_t) + ib);
  }
  if (symbol_blob) {
    uint32_t ib = 0; memcpy(&ib, img->base + (lssize_t)symbol_blob->file_off, sizeof(uint32_t));
    ybase = (const char*)(img->base + (lssize_t)symbol_blob->file_off + sizeof(uint32_t) + ib);
  }
  // parse pattern table if present
  uint32_t       pcnt = 0;
  const uint8_t* pt   = NULL;
  const uint64_t*po   = NULL;
  const uint8_t* pt_section = NULL; // section base (for entry offsets)
  if (pattern_tab) {
    uint32_t ib = 0; memcpy(&ib, img->base + (lssize_t)pattern_tab->file_off, sizeof(uint32_t));
    pt_section = img->base + (lssize_t)pattern_tab->file_off;
    pt         = pt_section + sizeof(uint32_t) + ib; // payload base
    memcpy(&pcnt, pt, sizeof(uint32_t));
    po = (const uint64_t*)(pt + sizeof(uint32_t));
  }
  for (uint32_t i = 0; i < ncnt; ++i) {
    const uint8_t* ent  = tt + offs[i];
    uint8_t        kind = ent[0];
  switch (kind) {
    case LSTB_KIND_INT: {
      uint32_t val = 0;
      memcpy(&val, ent + 8, sizeof(uint32_t));
      nodes[i] = lsthunk_new_int(lsint_new((int)val));
      break;
    }
    case LSTB_KIND_STR: {
      if (!string_blob) {
        free(nodes);
        return -EINVAL;
      }
      uint32_t len = 0, off = 0;
      memcpy(&len, ent + 4, sizeof(uint32_t));
      memcpy(&off, ent + 8, sizeof(uint32_t));
      const lsstr_t* s    = lsstr_new(sbase + off, (lssize_t)len);
      nodes[i]            = lsthunk_new_str(s);
      break;
    }
    case LSTB_KIND_SYMBOL: {
      if (!symbol_blob) {
        free(nodes);
        return -EINVAL;
      }
      uint32_t len = 0, off = 0;
      memcpy(&len, ent + 4, sizeof(uint32_t));
      memcpy(&off, ent + 8, sizeof(uint32_t));
      const lsstr_t* s    = lsstr_new(ybase + off, (lssize_t)len);
      nodes[i]            = lsthunk_new_symbol(s);
      break;
    }
    case LSTB_KIND_ALGE: {
      if (!symbol_blob) {
        free(nodes);
        free(pend);
        return -EINVAL;
      }
      // header fields
      uint32_t argc = 0, off = 0;
      memcpy(&argc, ent + 4, sizeof(uint32_t));
      memcpy(&off, ent + 8, sizeof(uint32_t));
      const uint8_t* p          = ent + 12; // payload starts after header
      uint32_t       constr_len = 0;
      memcpy(&constr_len, p, sizeof(uint32_t));
      p += 4;
      const lsstr_t* cstr = lsstr_new(ybase + off, (lssize_t)constr_len);
      // allocate via public API for two-phase wiring
      lsthunk_t* t = lsthunk_alloc_alge(cstr, (lssize_t)argc);
      nodes[i]     = t;
      if (argc > 0) {
        pend[i].kind = 1;
        pend[i].argc = (lssize_t)argc;
        pend[i].ids  = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)argc);
        if (!pend[i].ids) {
          free(pend);
          free(nodes);
          return -ENOMEM;
        }
        for (uint32_t j = 0; j < argc; ++j) {
          uint32_t idj = 0;
          memcpy(&idj, p, sizeof(uint32_t));
          p += 4;
          pend[i].ids[j] = idj;
        }
      }
      break;
    }
    case LSTB_KIND_BOTTOM: {
      if (!string_blob) {
        free(nodes);
        free(pend);
        return -EINVAL;
      }
      uint32_t rc = 0, off = 0;
      memcpy(&rc, ent + 4, sizeof(uint32_t));
      memcpy(&off, ent + 8, sizeof(uint32_t));
      const uint8_t* p       = ent + 12;
      uint32_t       msg_len = 0;
      memcpy(&msg_len, p, sizeof(uint32_t));
      p += 4;
      // copy message into a null-terminated C string
      char* m = (char*)lsmalloc((size_t)msg_len + 1);
      if (msg_len > 0)
        memcpy(m, sbase + off, (size_t)msg_len);
      m[msg_len] = '\0';
      // allocate with reserved related slots via API
      lsthunk_t* t = lsthunk_alloc_bottom(m, lstrace_take_pending_or_unknown(), (lssize_t)rc);
      nodes[i]     = t;
      if (rc > 0) {
        pend[i].kind = 2;
        pend[i].argc = (lssize_t)rc;
        pend[i].ids  = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)rc);
        if (!pend[i].ids) {
          free(pend);
          free(nodes);
          return -ENOMEM;
        }
        for (uint32_t j = 0; j < rc; ++j) {
          uint32_t idj = 0;
          memcpy(&idj, p, sizeof(uint32_t));
          p += 4;
          pend[i].ids[j] = idj;
        }
      }
      break;
    }
    case LSTB_KIND_APPL: {
      // header fields: aorl=argc, extra=func_id; payload: args ids
      uint32_t argc = 0, fid = 0;
      memcpy(&argc, ent + 4, sizeof(uint32_t));
      memcpy(&fid, ent + 8, sizeof(uint32_t));
      const uint8_t* p = ent + 12;
      lsthunk_t*     t = lsthunk_alloc_appl((lssize_t)argc);
      if (!t) {
        free(pend);
        free(nodes);
        return -ENOMEM;
      }
      nodes[i]     = t;
      pend[i].kind = 3;
      pend[i].argc = (lssize_t)argc;
      pend[i].extra = fid;
      if (argc > 0) {
        pend[i].ids = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)argc);
        if (!pend[i].ids) {
          free(pend);
          free(nodes);
          return -ENOMEM;
        }
        for (uint32_t j = 0; j < argc; ++j) {
          uint32_t idj = 0;
          memcpy(&idj, p, sizeof(uint32_t));
          p += 4;
          pend[i].ids[j] = idj;
        }
      } else {
        pend[i].ids = NULL;
      }
      break;
    }
    case LSTB_KIND_CHOICE: {
      // aorl=choice_kind; payload: left,right ids
      uint32_t ck = 0;
      memcpy(&ck, ent + 4, sizeof(uint32_t));
      const uint8_t* p = ent + 12;
      uint32_t       lid = 0, rid = 0;
      memcpy(&lid, p, sizeof(uint32_t));
      memcpy(&rid, p + 4, sizeof(uint32_t));
      lsthunk_t* t = lsthunk_alloc_choice((int)ck);
      if (!t) {
        free(pend);
        free(nodes);
        return -ENOMEM;
      }
      nodes[i]      = t;
      pend[i].kind  = 4;
      pend[i].argc  = 2;
      pend[i].ids   = (uint32_t*)malloc(sizeof(uint32_t) * 2);
      if (!pend[i].ids) {
        free(pend);
        free(nodes);
        return -ENOMEM;
      }
      pend[i].ids[0] = lid;
      pend[i].ids[1] = rid;
      break;
    }
    case LSTB_KIND_REF: {
      // aorl=len, extra=offset into SYMBOL_BLOB
      if (!symbol_blob) {
        free(pend);
        free(nodes);
        return -EINVAL;
      }
      uint32_t len = 0, off = 0;
      memcpy(&len, ent + 4, sizeof(uint32_t));
      memcpy(&off, ent + 8, sizeof(uint32_t));
  const lsstr_t* s    = lsstr_new(ybase + off, (lssize_t)len);
      const lsref_t* r    = lsref_new(s, lstrace_take_pending_or_unknown());
      nodes[i]            = lsthunk_new_ref(r, prelude_env);
      break;
    }
    case LSTB_KIND_BUILTIN: {
      // For v1 subset, BUILTIN entries are not emitted; writers encode as REF to name.
      // If encountered (forward-compat), treat like REF by name.
      if (!symbol_blob) {
        free(pend);
        free(nodes);
        return -EINVAL;
      }
      uint32_t len = 0, off = 0;
      memcpy(&len, ent + 4, sizeof(uint32_t));
      memcpy(&off, ent + 8, sizeof(uint32_t));
  const lsstr_t* s    = lsstr_new(ybase + off, (lssize_t)len);
      const lsref_t* r    = lsref_new(s, lstrace_take_pending_or_unknown());
      nodes[i]            = lsthunk_new_ref(r, prelude_env);
      break;
    }
    case LSTB_KIND_LAMBDA: {
      // Create placeholder; we'll wire body later. Param comes from pattern_tab
      if (!pattern_tab) {
        free(pend);
        free(nodes);
        return -EINVAL;
      }
      uint32_t pid = 0, bid = 0;
      memcpy(&pid, ent + 4, sizeof(uint32_t));
      memcpy(&bid, ent + 8, sizeof(uint32_t));
      if (pid >= pcnt || bid >= ncnt) {
        free(pend);
        free(nodes);
        return -EINVAL;
      }
      // decode pattern entry at pt+po[pid]
  const uint8_t* pp = pt_section + po[pid];
      uint8_t        pk = pp[0];
      // recursive lambda to decode pattern by index
      // We'll cache decoded patterns by index for potential sharing
      static lstpat_t** pdec = NULL;
      static uint32_t   pdec_cap = 0;
      if (pdec_cap < pcnt) {
        pdec = (lstpat_t**)realloc(pdec, sizeof(lstpat_t*) * pcnt);
        for (uint32_t x = pdec_cap; x < pcnt; ++x)
          pdec[x] = NULL;
        pdec_cap = pcnt;
      }
      // local recursive decoder
      lstpat_t* (*decode)(uint32_t) = NULL;
      lstpat_t* decode_impl(uint32_t idx) {
        if (idx >= pcnt)
          return NULL;
        if (pdec && pdec[idx])
          return pdec[idx];
  const uint8_t* ppe = pt_section + po[idx];
        uint8_t        kind = ppe[0];
        ppe++;
        switch (kind) {
        case LSPTYPE_ALGE: {
          uint32_t clen = 0, coff = 0, argc = 0;
          memcpy(&clen, ppe, 4);
          memcpy(&coff, ppe + 4, 4);
          memcpy(&argc, ppe + 8, 4);
          ppe += 12;
          const lsstr_t* constr = lsstr_new(ybase + coff, (lssize_t)clen);
          lstpat_t*      args = NULL;
          lstpat_t**     argp = NULL;
          if (argc > 0) {
            argp = (lstpat_t**)malloc(sizeof(lstpat_t*) * argc);
            for (uint32_t j = 0; j < argc; ++j) {
              uint32_t cidx = 0;
              memcpy(&cidx, ppe, 4);
              ppe += 4;
              argp[j] = decode_impl(cidx);
            }
          }
          lstpat_t* ret = lstpat_new_alge_raw(constr, (lssize_t)argc, argp);
          if (argp)
            free(argp);
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_AS: {
          uint32_t rid = 0, iid = 0;
          memcpy(&rid, ppe, 4);
          memcpy(&iid, ppe + 4, 4);
          lstpat_t* ref = decode_impl(rid);
          lstpat_t* in  = decode_impl(iid);
          lstpat_t* ret = lstpat_new_as_raw(ref, in);
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_INT: {
          int64_t v = 0;
          memcpy(&v, ppe, sizeof(v));
          lstpat_t* ret = lstpat_new_int_raw(lsint_new((int)v));
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_STR: {
          uint32_t slen = 0, soff = 0;
          memcpy(&slen, ppe, 4);
          memcpy(&soff, ppe + 4, 4);
          const lsstr_t* s   = lsstr_new(sbase + soff, (lssize_t)slen);
          lstpat_t*      ret = lstpat_new_str_raw(s);
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_REF: {
          uint32_t ylen = 0, yoff = 0;
          memcpy(&ylen, ppe, 4);
          memcpy(&yoff, ppe + 4, 4);
          const lsstr_t* s  = lsstr_new(ybase + yoff, (lssize_t)ylen);
          const lsref_t* rr = lsref_new(s, lstrace_take_pending_or_unknown());
          lstpat_t*      ret = lstpat_new_ref(rr);
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_WILDCARD: {
          lstpat_t* ret = lstpat_new_wild_raw();
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_OR: {
          uint32_t l = 0, r = 0;
          memcpy(&l, ppe, 4);
          memcpy(&r, ppe + 4, 4);
          lstpat_t* ret = lstpat_new_or_raw(decode_impl(l), decode_impl(r));
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        case LSPTYPE_CARET: {
          uint32_t i = 0;
          memcpy(&i, ppe, 4);
          lstpat_t* ret = lstpat_new_caret_raw(decode_impl(i));
          if (pdec)
            pdec[idx] = ret;
          return ret;
        }
        default:
          return NULL;
        }
      }
      // assign function pointer to call recursive impl
      decode = &decode_impl;
      lstpat_t* param = decode(pid);
      if (!param) {
        free(pend);
        free(nodes);
        return -EINVAL;
      }
  // Allocate lambda thunk via internal-friendly API
  lsthunk_t* lam = lsthunk_alloc_lambda(param);
      nodes[i]                 = lam;
      pend[i].kind             = 5; // lambda pending body
      pend[i].argc             = 1;
      pend[i].ids              = (uint32_t*)malloc(sizeof(uint32_t));
      if (!pend[i].ids) {
        free(pend);
        free(nodes);
        return -ENOMEM;
      }
      pend[i].ids[0] = bid;
      break;
    }
    default: {
      // Not supported in this subset
      for (uint32_t k = 0; k < i; ++k) { /* GC managed by Boehm */
      }
      free(pend);
      free(nodes);
      return -ENOSYS;
    }
    }
  }
  // Second pass: wire pending edges
  for (uint32_t i = 0; i < ncnt; ++i) {
    if ((pend[i].argc > 0 && pend[i].ids) || pend[i].kind == 3 || pend[i].kind == 4 ||
        pend[i].kind == 5) {
      if (pend[i].kind == 1 && lsthunk_get_type(nodes[i]) == LSTTYPE_ALGE) {
        for (lssize_t j = 0; j < pend[i].argc; ++j)
          lsthunk_set_alge_arg(nodes[i], j, nodes[pend[i].ids[j]]);
      } else if (pend[i].kind == 2 && lsthunk_get_type(nodes[i]) == LSTTYPE_BOTTOM) {
        for (lssize_t j = 0; j < pend[i].argc; ++j)
          lsthunk_set_bottom_related(nodes[i], j, nodes[pend[i].ids[j]]);
      } else if (pend[i].kind == 3 && lsthunk_get_type(nodes[i]) == LSTTYPE_APPL) {
        lsthunk_set_appl_func(nodes[i], nodes[pend[i].extra]);
        for (lssize_t j = 0; j < pend[i].argc; ++j)
          lsthunk_set_appl_arg(nodes[i], j, nodes[pend[i].ids[j]]);
      } else if (pend[i].kind == 4 && lsthunk_get_type(nodes[i]) == LSTTYPE_CHOICE) {
        lsthunk_set_choice_left(nodes[i], nodes[pend[i].ids[0]]);
        lsthunk_set_choice_right(nodes[i], nodes[pend[i].ids[1]]);
      } else if (pend[i].kind == 5 && lsthunk_get_type(nodes[i]) == LSTTYPE_LAMBDA) {
        // set body
        lsthunk_set_lambda_body(nodes[i], nodes[pend[i].ids[0]]);
      }
      if (pend[i].ids) {
        free(pend[i].ids);
        pend[i].ids = NULL;
      }
      pend[i].argc = 0;
    }
  }
  // Extract roots
  const uint8_t* rp   = img->base + (lssize_t)roots->file_off;
  uint32_t       rcnt = 0;
  memcpy(&rcnt, rp, sizeof(uint32_t));
  const uint32_t* rids = (const uint32_t*)(rp + sizeof(uint32_t));
  lsthunk_t**     r    = (lsthunk_t**)calloc(rcnt, sizeof(lsthunk_t*));
  if (!r) {
    free(pend);
    free(nodes);
    return -ENOMEM;
  }
  for (uint32_t i = 0; i < rcnt; ++i)
    r[i] = nodes[rids[i]];
  *out_roots = r;
  *out_rootc = (lssize_t)rcnt;
  free(pend);
  free(nodes);
  return 0;
}

int lsti_unmap(lsti_image_t* img) {
  if (!img)
    return -EINVAL;
  free((void*)img->base);
  img->base = NULL;
  img->size = 0;
  return 0;
}

#endif

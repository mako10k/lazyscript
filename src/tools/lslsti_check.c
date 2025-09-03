#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lazyscript.h"
#include "thunk/thunk.h"
#include "thunk/lsti.h"
#include "thunk/thunk_bin.h"
#include "expr/expr.h"
#include "expr/ealge.h"
#include "common/malloc.h"
#include "runtime/trace.h"
#include <getopt.h>
#include "lazyscript.h"
// Note: We don't need real runtime builtins here; provide a local dummy.

// Local dummy builtin function (never actually evaluated in this smoke)
static lsthunk_t* lsdummy_builtin(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)args;
  (void)data;
  return lsthunk_bottom_here("dummy-builtin should not run in lslsti_check");
}

int main(int argc, char** argv) {
  const char*   path = "./_tmp_test.lsti";
  int           cli_opt;
  struct option longopts[] = {
    { "output", required_argument, NULL, 'o' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 },
  };
  while ((cli_opt = getopt_long(argc, argv, "o:hv", longopts, NULL)) != -1) {
    switch (cli_opt) {
    case 'o':
      path = optarg;
      break;
    case 'h':
      printf("Usage: %s [-o FILE]\n", argv[0]);
      printf("  -o, --output FILE   output LSTI path (default: ./_tmp_test.lsti)\n");
      printf("  -h, --help          show this help and exit\n");
      printf("  -v, --version       print version and exit\n");
      return 0;
    case 'v':
      printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      return 0;
    default:
      fprintf(stderr, "Try --help for usage.\n");
      return 1;
    }
  }
  if (optind < argc)
    path = argv[optind++];
  // Build a tiny graph: INT 42, STR "hello", SYMBOL ".Ok", ALGE (.Ok 42), BOTTOM("boom",
  // related=[INT 42])
  lstenv_t*      env     = lstenv_new(NULL);
  const lsint_t* i42     = lsint_new(42);
  lsthunk_t*     ti      = lsthunk_new_int(i42);
  const lsstr_t* s_hello = lsstr_cstr("hello");
  lsthunk_t*     ts      = lsthunk_new_str(s_hello);
  const lsstr_t* y_ok    = lsstr_cstr(".Ok");
  lsthunk_t*     ty      = lsthunk_new_symbol(y_ok);
  // ALGE: (.Ok 42) using public APIs
  const lsexpr_t*  arg_e    = lsexpr_new_int(i42);
  const lsexpr_t*  args1[1] = { arg_e };
  const lsealge_t* e_ok     = lsealge_new(lsstr_cstr(".Ok"), 1, args1);
  lsthunk_t*       talge    = lsthunk_new_ealge(e_ok, env);
  // BOTTOM: message and one related using public API
  lsthunk_t* related[1] = { ts };
  lsthunk_t* tbot       = lsthunk_new_bottom("boom", lstrace_take_pending_or_unknown(), 1, related);
  // BUILTIN: prelude.print (arity 1) — writer will encode as REF("prelude.print")
  // Use a local dummy func to avoid linking real builtins.
  lsthunk_t* tbi =
      lsthunk_new_builtin_attr(lsstr_cstr("prelude.print"), 1, lsdummy_builtin, NULL, 0);
  // LAMBDA: \x -> x  （param = REF x, body = REF x）
  const lsref_t* xr   = lsref_new(lsstr_cstr("x"), lstrace_take_pending_or_unknown());
  lstpat_t*      xpat = lstpat_new_ref(xr);
  lsthunk_t*     lam  = lsthunk_alloc_lambda(xpat);
  lsthunk_t*     xref = lsthunk_new_ref(xr, env);
  lsthunk_set_lambda_body(lam, xref);
  lsthunk_t* roots[7] = { ti, ts, ty, talge, tbot, tbi, lam };

  FILE*      fp = fopen(path, "wb");
  if (!fp) {
    perror("fopen");
    return 1;
  }
  lsti_write_opts_t opt = { .align_log2 = LSTI_ALIGN_8, .flags = 0 };
  int               rc  = lsti_write(fp, roots, 7, &opt);
  fclose(fp);
  if (rc != 0) {
    fprintf(stderr, "lsti_write rc=%d\n", rc);
    return 2;
  }

  lsti_image_t img = { 0 };
  rc               = lsti_map(path, &img);
  if (rc != 0) {
    fprintf(stderr, "lsti_map rc=%d\n", rc);
    return 3;
  }
  // Quick introspection dump before validate
  {
    typedef struct {
      uint32_t magic;
      uint16_t vmaj, vmin;
      uint16_t sect_cnt;
      uint16_t align_log2;
      uint32_t flags;
    } test_lsti_hdr_t;
    typedef struct {
      uint32_t kind;
      uint32_t reserved;
      uint64_t file_off;
      uint64_t size;
    } test_lsti_sect_t;
    const uint8_t*          base = img.base;
    const test_lsti_hdr_t*  hdr  = (const test_lsti_hdr_t*)base;
    const test_lsti_sect_t* sect = (const test_lsti_sect_t*)(base + sizeof(test_lsti_hdr_t));
    fprintf(stderr, "dump: sect_cnt=%u align=%u size=%ld\n", (unsigned)hdr->sect_cnt,
            (unsigned)hdr->align_log2, (long)img.size);
    for (uint16_t i = 0; i < hdr->sect_cnt; ++i) {
      fprintf(stderr, "  sect[%u]: kind=%u off=%llu sz=%llu\n", (unsigned)i, (unsigned)sect[i].kind,
              (unsigned long long)sect[i].file_off, (unsigned long long)sect[i].size);
      if (sect[i].kind == (uint32_t)LSTI_SECT_STRING_BLOB ||
          sect[i].kind == (uint32_t)LSTI_SECT_SYMBOL_BLOB) {
        uint32_t ib = 0;
        memcpy(&ib, base + (lssize_t)sect[i].file_off, sizeof(uint32_t));
        fprintf(stderr, "    blob index_bytes=%u\n", (unsigned)ib);
        if (sect[i].size >= 4 + ib) {
          uint32_t cnt = 0;
          memcpy(&cnt, base + (lssize_t)sect[i].file_off + 4, sizeof(uint32_t));
          fprintf(stderr, "    blob count=%u\n", (unsigned)cnt);
        }
      }
      if (sect[i].kind == (uint32_t)LSTI_SECT_PATTERN_TAB) {
        uint32_t ib = 0;
        memcpy(&ib, base + (lssize_t)sect[i].file_off, sizeof(uint32_t));
        fprintf(stderr, "    patt index_bytes=%u\n", (unsigned)ib);
        if (sect[i].size >= 4 + ib) {
          uint32_t pc = 0;
          memcpy(&pc, base + (lssize_t)sect[i].file_off + 4, sizeof(uint32_t));
          fprintf(stderr, "    patt count=%u\n", (unsigned)pc);
        }
      }
      if (sect[i].kind == (uint32_t)LSTI_SECT_THUNK_TAB) {
        uint32_t n = 0;
        memcpy(&n, base + (lssize_t)sect[i].file_off, sizeof(uint32_t));
        fprintf(stderr, "    thunk count=%u\n", (unsigned)n);
        const uint64_t* offs = (const uint64_t*)(base + (lssize_t)sect[i].file_off + 4);
        for (uint32_t k = 0; k < n && k < 8; ++k) {
          const uint8_t* ent = base + (lssize_t)sect[i].file_off + (lssize_t)offs[k];
          fprintf(stderr, "      ent[%u] kind=%u aorl=%u extra=%u\n", (unsigned)k, (unsigned)ent[0],
                  (unsigned)*(uint32_t*)(ent + 4), (unsigned)*(uint32_t*)(ent + 8));
        }
      }
    }
  }
  rc = lsti_validate(&img);
  if (rc != 0) {
    fprintf(stderr, "lsti_validate rc=%d\n", rc);
    lsti_unmap(&img);
    return 4;
  }
  // Try materialize (now supports INT/STR/SYMBOL/ALGE/BOTTOM/APPL/CHOICE/REF/LAMBDA subset)
  lsthunk_t** out_roots = NULL;
  lssize_t    outc      = 0;
  rc                    = lsti_materialize(&img, &out_roots, &outc, env);
  if (rc == 0) {
    printf("materialize: roots=%ld\n", (long)outc);
  } else {
    printf("materialize: rc=%d (expected for subset)\n", rc);
  }
  // Roundtrip: write materialized roots to another file and compare bytes
  if (rc == 0 && out_roots && outc > 0) {
    const char* path2 = "./_tmp_test2.lsti";
    FILE*       fp2   = fopen(path2, "wb");
    if (!fp2) {
      perror("fopen2");
      return 5;
    }
    lsti_write_opts_t opt2 = { .align_log2 = LSTI_ALIGN_8, .flags = 0 };
    int               rc2  = lsti_write(fp2, out_roots, outc, &opt2);
    fclose(fp2);
    if (rc2 != 0) {
      fprintf(stderr, "lsti_write (roundtrip) rc=%d\n", rc2);
      return 6;
    }
    // Compare files byte-by-byte
    FILE* a = fopen(path, "rb");
    FILE* b = fopen(path2, "rb");
    if (!a || !b) {
      perror("fopen-compare");
      return 7;
    }
    int equal = 1;
    for (;;) {
      unsigned char ba[4096], bb[4096];
      size_t        ra = fread(ba, 1, sizeof ba, a);
      size_t        rb = fread(bb, 1, sizeof bb, b);
      if (ra != rb) {
        equal = 0;
        break;
      }
      if (ra == 0)
        break;
      if (memcmp(ba, bb, ra) != 0) {
        equal = 0;
        break;
      }
    }
    fclose(a);
    fclose(b);
    printf("roundtrip: %s\n", equal ? "equal" : "different");
  }
  lsti_unmap(&img);
  (void)env; // quiet unused for now
  printf("ok: %s\n", path);
  // Minimal negative tests: mutate a few bytes to trigger validator failures.
  // Local copies of on-disk structs (must match lsti.c layout)
  typedef struct {
    uint32_t magic;
    uint16_t vmaj, vmin;
    uint16_t sect_cnt;
    uint16_t align_log2;
    uint32_t flags;
  } test_lsti_hdr_t;
  typedef struct {
    uint32_t kind;
    uint32_t reserved;
    uint64_t file_off;
    uint64_t size;
  } test_lsti_sect_t;
  // 1) Corrupt ROOTS count to exceed ncnt
  {
    lsti_image_t bad = img;
    // duplicate buffer to mutate safely
    uint8_t* dup = (uint8_t*)malloc((size_t)img.size);
    memcpy(dup, img.base, (size_t)img.size);
    bad.base                          = dup;
    const test_lsti_hdr_t*  hdr       = (const test_lsti_hdr_t*)dup;
    const test_lsti_sect_t* sect      = (const test_lsti_sect_t*)(dup + sizeof(test_lsti_hdr_t));
    const test_lsti_sect_t* thunk_tab = NULL;
    const test_lsti_sect_t* roots     = NULL;
    for (uint16_t i = 0; i < hdr->sect_cnt; ++i) {
      if (sect[i].kind == (uint32_t)LSTI_SECT_THUNK_TAB)
        thunk_tab = &sect[i];
      if (sect[i].kind == (uint32_t)LSTI_SECT_ROOTS)
        roots = &sect[i];
    }
    if (thunk_tab && roots) {
      const uint8_t* tt   = dup + (lssize_t)thunk_tab->file_off;
      uint32_t       ncnt = 0;
      memcpy(&ncnt, tt, sizeof(uint32_t));
      uint8_t* rp     = dup + (lssize_t)roots->file_off;
      uint32_t badcnt = ncnt + 1;
      memcpy(rp, &badcnt, sizeof(uint32_t));
      int rc_bad = lsti_validate(&bad);
      printf("neg1 (roots>ncnt): %s\n", rc_bad != 0 ? "fail as expected" : "UNEXPECTED PASS");
    }
    free(dup);
  }
  // 2) If STRING_BLOB present, set STR node's len to exceed payload
  {
    lsti_image_t bad = img;
    uint8_t*     dup = (uint8_t*)malloc((size_t)img.size);
    memcpy(dup, img.base, (size_t)img.size);
    bad.base                          = dup;
    const test_lsti_hdr_t*  hdr       = (const test_lsti_hdr_t*)dup;
    const test_lsti_sect_t* sect      = (const test_lsti_sect_t*)(dup + sizeof(test_lsti_hdr_t));
    const test_lsti_sect_t* thunk_tab = NULL;
    const test_lsti_sect_t* sblob     = NULL;
    for (uint16_t i = 0; i < hdr->sect_cnt; ++i) {
      if (sect[i].kind == (uint32_t)LSTI_SECT_THUNK_TAB)
        thunk_tab = &sect[i];
      if (sect[i].kind == (uint32_t)LSTI_SECT_STRING_BLOB)
        sblob = &sect[i];
    }
    if (thunk_tab && sblob) {
      uint32_t ib = 0;
      memcpy(&ib, dup + (lssize_t)sblob->file_off, sizeof(uint32_t));
      uint64_t s_payload = sblob->size - (uint64_t)sizeof(uint32_t) - (uint64_t)ib;
      uint8_t* tt        = dup + (lssize_t)thunk_tab->file_off;
      uint32_t ncnt      = 0;
      memcpy(&ncnt, tt, sizeof(uint32_t));
      uint64_t* offs = (uint64_t*)(tt + sizeof(uint32_t));
      for (uint32_t i = 0; i < ncnt; ++i) {
        uint8_t* ent = tt + offs[i];
        if (ent[0] == (uint8_t)LSTB_KIND_STR) {
          uint32_t too_big = (uint32_t)(s_payload + 1);
          memcpy(ent + 4, &too_big, sizeof(uint32_t));
          break;
        }
      }
      int rc_bad = lsti_validate(&bad);
      printf("neg2 (str overflow): %s\n", rc_bad != 0 ? "fail as expected" : "UNEXPECTED PASS");
    }
    free(dup);
  }
  return 0;
}
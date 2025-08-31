#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lazyscript.h"
#include "thunk/thunk.h"
#include "thunk/lsti.h"

int main(int argc, char** argv) {
  const char* path = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  // Build a tiny graph: INT 42, STR "hello", SYMBOL ".Ok"
  lstenv_t* env = lstenv_new(NULL);
  const lsint_t* i42 = lsint_new(42);
  lsthunk_t* ti = lsthunk_new_int(i42);
  const lsstr_t* s_hello = lsstr_cstr("hello");
  lsthunk_t* ts = lsthunk_new_str(s_hello);
  const lsstr_t* y_ok = lsstr_cstr(".Ok");
  lsthunk_t* ty = lsthunk_new_symbol(y_ok);
  lsthunk_t* roots[3] = { ti, ts, ty };

  FILE* fp = fopen(path, "wb");
  if (!fp) { perror("fopen"); return 1; }
  lsti_write_opts_t opt = { .align_log2 = LSTI_ALIGN_8, .flags = 0 };
  int rc = lsti_write(fp, roots, 3, &opt);
  fclose(fp);
  if (rc != 0) { fprintf(stderr, "lsti_write rc=%d\n", rc); return 2; }

  lsti_image_t img = {0};
  rc = lsti_map(path, &img);
  if (rc != 0) { fprintf(stderr, "lsti_map rc=%d\n", rc); return 3; }
  rc = lsti_validate(&img);
  if (rc != 0) { fprintf(stderr, "lsti_validate rc=%d\n", rc); lsti_unmap(&img); return 4; }
  // Try materialize (INT/STR/SYMBOL subset)
  lsthunk_t** out_roots = NULL; lssize_t outc = 0;
  rc = lsti_materialize(&img, &out_roots, &outc, env);
  if (rc == 0) {
    printf("materialize: roots=%ld\n", (long)outc);
  } else {
    printf("materialize: rc=%d (expected for subset)\n", rc);
  }
  lsti_unmap(&img);
  (void)env; // quiet unused for now
  printf("ok: %s\n", path);
  return 0;
}
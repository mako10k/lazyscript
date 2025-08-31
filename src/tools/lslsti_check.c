#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lazyscript.h"
#include "thunk/thunk.h"
#include "thunk/lsti.h"

int main(int argc, char** argv) {
  const char* path = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  // Build a tiny graph: ALGE(Constr: .Ok, args: [INT 42])
  lstenv_t* env = lstenv_new(NULL);
  const lsint_t* i42 = lsint_new(42);
  lsthunk_t* ti = lsthunk_new_int(i42);
  // Build constructor symbol .Ok via string helpers
  // const lsstr_t* ok = lsstr_cstr(".Ok");
  // Fake an ALGE thunk via expr builder if available; fallback print INT only
  // Here we just write roots={INT} to avoid depending on expr constructors
  lsthunk_t* roots[1] = { ti };

  FILE* fp = fopen(path, "wb");
  if (!fp) { perror("fopen"); return 1; }
  lsti_write_opts_t opt = { .align_log2 = LSTI_ALIGN_8, .flags = 0 };
  int rc = lsti_write(fp, roots, 1, &opt);
  fclose(fp);
  if (rc != 0) { fprintf(stderr, "lsti_write rc=%d\n", rc); return 2; }

  lsti_image_t img = {0};
  rc = lsti_map(path, &img);
  if (rc != 0) { fprintf(stderr, "lsti_map rc=%d\n", rc); return 3; }
  rc = lsti_validate(&img);
  if (rc != 0) { fprintf(stderr, "lsti_validate rc=%d\n", rc); lsti_unmap(&img); return 4; }
  lsti_unmap(&img);
  (void)env; // quiet unused for now
  printf("ok: %s\n", path);
  return 0;
}
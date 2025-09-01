#include <stdio.h>
#include "lstr/lstr.h"

int main(int argc, char** argv) {
  const char* path = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  const lstr_prog_t* p = lstr_from_lsti_path(path);
  if (!p) {
    fprintf(stderr, "failed: lstr_from_lsti_path(%s)\n", path);
    return 1;
  }
  if (lstr_validate(p) != 0) {
    fprintf(stderr, "invalid LSTR\n");
    return 2;
  }
  lstr_print(stdout, 0, p);
  return 0;
}

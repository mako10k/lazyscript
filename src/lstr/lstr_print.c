#include "lstr.h"
#include <stdio.h>

struct lstr_prog { int dummy; };

void lstr_print(FILE* out, int indent, const lstr_prog_t* p) {
  (void)indent; (void)p;
  fprintf(out, "; LSTR v1\n(prog)\n");
}

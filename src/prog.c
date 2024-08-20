#include "prog.h"

struct lsprog {
  lsexpr_t *expr;
};

lsprog_t *lsprog(lsexpr_t *expr) {
  lsprog_t *prog = malloc(sizeof(lsprog_t));
  prog->expr = expr;
  return prog;
}

void lsprog_print(FILE *fp, const lsprog_t *prog) {
  lsexpr_print(fp, 0, prog->expr);
}
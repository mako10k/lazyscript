#include "prog.h"
#include "lazyscript.h"

struct lsprog {
  lsexpr_t *expr;
};

lsprog_t *lsprog(lsexpr_t *expr) {
  lsprog_t *prog = malloc(sizeof(lsprog_t));
  prog->expr = expr;
  return prog;
}

void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog) {
  lsexpr_print(fp, prec, indent, prog->expr);
  lsprintf(fp, 0, ";\n");
}
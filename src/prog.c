#include "prog.h"
#include "io.h"
#include <assert.h>

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

int lsprog_prepare(lsprog_t *prog, lseenv_t *env) {
  return lsexpr_prepare(prog->expr, env);
}

lsthunk_t *lsprog_thunk(lstenv_t *tenv, const lsprog_t *prog) {
  return lsexpr_thunk(tenv, prog->expr);
}
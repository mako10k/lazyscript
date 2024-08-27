#include "prog.h"
#include "io.h"
#include "malloc.h"
#include <assert.h>

struct lsprog {
  lsexpr_t *lprg_expr;
};

lsprog_t *lsprog_new(lsexpr_t *expr) {
  lsprog_t *prog = lsmalloc(sizeof(lsprog_t));
  prog->lprg_expr = expr;
  return prog;
}

void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog) {
  lsexpr_print(fp, prec, indent, prog->lprg_expr);
  lsprintf(fp, 0, ";\n");
}

int lsprog_prepare(lsprog_t *prog, lseenv_t *env) {
  return lsexpr_prepare(prog->lprg_expr, env);
}

lsthunk_t *lsprog_thunk(lstenv_t *tenv, const lsprog_t *prog) {
  return lsexpr_thunk(tenv, prog->lprg_expr);
}
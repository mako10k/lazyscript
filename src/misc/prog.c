#include "misc/prog.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lsprog {
  lsexpr_t *lp_expr;
};

struct lsscan {
  lsprog_t *ls_prog;
  const char *ls_filename;
};

lsprog_t *lsprog_new(lsexpr_t *expr) {
  lsprog_t *prog = lsmalloc(sizeof(lsprog_t));
  prog->lp_expr = expr;
  return prog;
}

void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog) {
  lsexpr_print(fp, prec, indent, prog->lp_expr);
  lsprintf(fp, 0, ";\n");
}

int lsprog_prepare(lsprog_t *prog, lseenv_t *env) {
  return lsexpr_prepare(prog->lp_expr, env);
}

lsthunk_t *lsprog_thunk(lstenv_t *tenv, const lsprog_t *prog) {
  return lsthunk_new_expr(tenv, prog->lp_expr);
}

void yyerror(lsloc_t *loc, lsscan_t *scanner, const char *s) {
  (void)scanner;
  lsloc_print(stderr, *loc);
  lsprintf(stderr, 0, "%s\n", s);
}

lsscan_t *lsscan_new(const char *filename) {
  lsscan_t *scanner = lsmalloc(sizeof(lsscan_t));
  scanner->ls_prog = NULL;
  scanner->ls_filename = filename;
  return scanner;
}

lsprog_t *lsscan_get_prog(const lsscan_t *scanner) { return scanner->ls_prog; }

void lsscan_set_prog(lsscan_t *scanner, lsprog_t *prog) {
  scanner->ls_prog = prog;
}

const char *lsscan_get_filename(const lsscan_t *scanner) {
  return scanner->ls_filename;
}
#pragma once

typedef struct lsprog lsprog_t;
typedef struct lsscan lsscan_t;

#include "expr/eenv.h"
#include "expr/expr.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"

lsprog_t *lsprog_new(lsexpr_t *expr);
void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog);
int lsprog_prepare(lsprog_t *prog, lseenv_t *env);
lsthunk_t *lsprog_thunk(lstenv_t *tenv, const lsprog_t *prog);

void yyerror(lsloc_t *loc, lsscan_t *scanner, const char *s);

lsscan_t *lsscan_new(const char *filename);
lsprog_t *lsscan_get_prog(const lsscan_t *scanner);
void lsscan_set_prog(lsscan_t *scanner, lsprog_t *prog);
const char *lsscan_get_filename(const lsscan_t *scanner);

#pragma once

typedef struct lsprog lsprog_t;

#include "eenv.h"
#include "expr.h"
#include "thunk.h"
#include "tenv.h"

lsprog_t *lsprog(lsexpr_t *expr);
void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog);
int lsprog_prepare(lsprog_t *prog, lseenv_t *env);
lsthunk_t *lsprog_thunk(lstenv_t *tenv, const lsprog_t *prog);
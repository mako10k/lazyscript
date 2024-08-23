#pragma once

typedef struct lsprog lsprog_t;

#include "env.h"
#include "expr.h"

lsprog_t *lsprog(lsexpr_t *expr);
void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog);
int lsprog_prepare(lsprog_t *prog, lsenv_t *env);
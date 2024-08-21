#pragma once

typedef struct lsprog lsprog_t;

#include "expr.h"

lsprog_t *lsprog(lsexpr_t *expr);

void lsprog_print(FILE *fp, int prec, int indent, const lsprog_t *prog);
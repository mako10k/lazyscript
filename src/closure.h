#pragma once

typedef struct lsclosure lsclosure_t;

#include "bind.h"
#include "expr.h"

lsclosure_t *lsclosure(lsexpr_t *expr, lsbind_t *bind);

void lsclosure_print(FILE *fp, int prec, int indent, lsclosure_t *closure);
int lsclosure_prepare(lsclosure_t *closure, lsenv_t *env);
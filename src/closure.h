#pragma once

typedef struct lsclosure lsclosure_t;

#include "expr.h"
#include "bind.h"

lsclosure_t *lsclosure(lsexpr_t *expr, lsbind_t *bind);

void lsclosure_print(FILE *fp, int prec, int indent, lsclosure_t *closure);
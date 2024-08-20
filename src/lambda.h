#pragma once

typedef struct lslambda lslambda_t;

#include "expr.h"
#include "pat.h"
#include <stdio.h>

lslambda_t *lslambda(const lspat_t *pat, const lsexpr_t *expr);
void lslambda_print(FILE *fp, int prec, const lslambda_t *lambda);
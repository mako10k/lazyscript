#pragma once

typedef struct lslambda lslambda_t;

#include "expr.h"
#include "pat.h"

lslambda_t *lslambda(const lspat_t *pat, const lsexpr_t *expr);
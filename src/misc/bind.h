#pragma once

typedef struct lsbind lsbind_t;
typedef struct lsbind lsbind_t;

#include "expr/expr.h"
#include "pat/pat.h"

const lsbind_t *lsbind_new(const lspat_t *lhs, const lsexpr_t *rhs);
void lsbind_print(FILE *fp, lsprec_t prec, int indent,
                        const lsbind_t *ent);
const lspat_t *lsbind_get_lhs(const lsbind_t *ent);
const lsexpr_t *lsbind_get_rhs(const lsbind_t *ent);

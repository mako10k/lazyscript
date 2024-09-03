#pragma once

typedef struct lsbind lsbind_t;
typedef struct lsbind lsbind_t;

#include "expr/expr.h"
#include "pat/pat.h"

#define lsapi_bind_new lsapi_nn12 lsapi_wur

lsapi_bind_new const lsbind_t *lsbind_new(const lspat_t *lhs,
                                          const lsexpr_t *rhs);

lsapi_get const lspat_t *lsbind_get_lhs(const lsbind_t *ent);

lsapi_get const lsexpr_t *lsbind_get_rhs(const lsbind_t *ent);

lsapi_print void lsbind_print(FILE *fp, lsprec_t prec, int indent,
                              const lsbind_t *ent);

#pragma once

typedef struct lselambda lselambda_t;

#include "expr/expr.h"
#include "pat/pat.h"
#include <stdio.h>

lselambda_t *lselambda_new(lspat_t *arg, lsexpr_t *body);
lspat_t *lselambda_get_arg(const lselambda_t *lent);
lsexpr_t *lselambda_get_body(const lselambda_t *lent);
void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *lent);
lspres_t lselambda_prepare(lselambda_t *lent, lseenv_t *eenv);
#pragma once

typedef struct lselambda lselambda_t;

#include "expr/expr.h"
#include "pat/pat.h"
#include <stdio.h>

lselambda_t *lselambda_new(const lspat_t *arg, lsexpr_t *body);
const lspat_t *lselambda_get_arg(const lselambda_t *elambda);
lsexpr_t *lselambda_get_body(const lselambda_t *elambda);
void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *elambda);
lspres_t lselambda_prepare(lselambda_t *elambda, lseenv_t *eenv);
#pragma once

typedef struct lselambda lselambda_t;
typedef struct lselambda_entry lselambda_entry_t;

#include "expr.h"
#include "pat.h"
#include <stdio.h>

lselambda_entry_t *lselambda_entry_new(lspat_t *arg, lsexpr_t *body);
lspat_t *lselambda_entry_get_arg(const lselambda_entry_t *lent);
lsexpr_t *lselambda_entry_get_body(const lselambda_entry_t *lent);
void lselambda_entry_print(FILE *fp, lsprec_t prec, int indent,
                           const lselambda_entry_t *lent);
lspres_t lselambda_entry_prepare(lselambda_entry_t *lent, lseenv_t *eenv);

lselambda_t *lselambda_new(void);
lselambda_t *lselambda_push(lselambda_t *elambda, lselambda_entry_t *lent);
lssize_t lselambda_get_entry_count(const lselambda_t *elambda);
lselambda_entry_t *lselambda_get_ent(const lselambda_t *elambda, lssize_t i);
void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *elambda);
lspres_t lselambda_prepare(lselambda_t *elambda, lseenv_t *eenv);
#pragma once

typedef struct lselambda lselambda_t;
typedef struct lselambda_ent lselambda_ent_t;

#include "expr.h"
#include "pat.h"
#include "tenv.h"
#include "thunk.h"
#include <stdio.h>

lselambda_t *lselambda(void);
lselambda_ent_t *lselambda_ent(lspat_t *pat, lsexpr_t *expr);
lselambda_t *lselambda_push(lselambda_t *lambda, lselambda_ent_t *ent);
unsigned int lselambda_get_count(const lselambda_t *lambda);
void lselambda_print(FILE *fp, int prec, int indent, const lselambda_t *lambda);
void lselambda_ent_print(FILE *fp, int prec, int indent,
                         const lselambda_ent_t *ent);
int lselambda_prepare(lselambda_t *lambda, lseenv_t *env);
int lselambda_ent_prepare(lselambda_ent_t *ent, lseenv_t *env);
lsthunk_t *lselambda_thunk(lstenv_t *tenv, const lselambda_t *lambda);
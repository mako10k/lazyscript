#pragma once

typedef struct lslambda lslambda_t;
typedef struct lslambda_ent lslambda_ent_t;

#include "expr.h"
#include "pat.h"
#include <stdio.h>

lslambda_t *lslambda(void);
lslambda_ent_t *lslambda_ent(lspat_t *pat, lsexpr_t *expr);
lslambda_t *lslambda_push(lslambda_t *lambda, lslambda_ent_t *ent);
unsigned int lslambda_get_count(const lslambda_t *lambda);
void lslambda_print(FILE *fp, int prec, int indent, const lslambda_t *lambda);
void lslambda_ent_print(FILE *fp, int prec, int indent,
                        const lslambda_ent_t *ent);
int lslambda_prepare(lslambda_t *lambda, lsenv_t *env);
int lslambda_ent_prepare(lslambda_ent_t *ent, lsenv_t *env);
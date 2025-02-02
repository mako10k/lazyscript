#pragma once

typedef struct lslambda lslambda_t;
typedef struct lslambda_ent lslambda_ent_t;

#include "expr.h"
#include "pat.h"
#include <stdio.h>

lslambda_t *lslambda(void);
lslambda_ent_t *lslambda_ent(const lspat_t *pat, const lsexpr_t *expr);
lslambda_t *lslambda_push(lslambda_t *lambda, lslambda_ent_t *ent);
unsigned int lslambda_get_count(const lslambda_t *lambda);

void lslambda_print(FILE *fp, int prec, int indent, const lslambda_t *lambda);
void lslambda_ent_print(FILE *fp, int prec, int indent, const lslambda_ent_t *ent);
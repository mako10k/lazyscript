#pragma once

typedef struct lsbind lsbind_t;
typedef struct lsbind_ent lsbind_ent_t;

#include "expr.h"
#include "pat.h"

lsbind_t *lsbind(void);
void lsbind_push(lsbind_t *bind, lsbind_ent_t *ent);
lsbind_ent_t *lsbind_ent(lspat_t *pat, lsexpr_t *expr);
unsigned int lsbind_get_count(const lsbind_t *bind);
lsbind_ent_t *lsbind_get_ent(const lsbind_t *bind, unsigned int i);
lspat_t * lsbind_ent_get_pat(const lsbind_ent_t *ent);
lsexpr_t * lsbind_ent_get_expr(const lsbind_ent_t *ent);
void lsbind_print(FILE *fp, int prec, int indent, lsbind_t *bind);
void lsbind_ent_print(FILE *fp, int prec, int indent, lsbind_ent_t *ent);
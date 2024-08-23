#pragma once

#include <stdio.h>

typedef struct lsealge lsealge_t;

#include "array.h"
#include "env.h"
#include "expr.h"
#include "str.h"

lsealge_t *lsealge(const lsstr_t *constr);
void lsealge_push_arg(lsealge_t *alge, lsexpr_t *arg);
void lsealge_push_args(lsealge_t *alge, const lsarray_t *args);
const lsstr_t *lsealge_get_constr(const lsealge_t *alge);
unsigned int lsealge_get_argc(const lsealge_t *alge);
lsexpr_t *lsealge_get_arg(const lsealge_t *alge, int i);
void lsealge_print(FILE *fp, int prec, int indent, const lsealge_t *alge);
int lsealge_prepare(lsealge_t *alge, lsenv_t *env);
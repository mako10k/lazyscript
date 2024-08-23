#pragma once

#include <stdio.h>
typedef struct lsappl lsappl_t;

#include "array.h"
#include "env.h"
#include "expr.h"

lsappl_t *lsappl(const lsexpr_t *func);
void lsappl_push_arg(lsappl_t *appl, lsexpr_t *arg);
void lsappl_push_args(lsappl_t *appl, lsarray_t *args);
const lsexpr_t *lsappl_get_func(const lsappl_t *appl);
unsigned int lsappl_get_argc(const lsappl_t *appl);
lsexpr_t *lsappl_get_arg(const lsappl_t *appl, unsigned int i);
void lsappl_print(FILE *fp, int prec, int indent, const lsappl_t *appl);
int lsappl_prepare(lsappl_t *appl, lsenv_t *env);
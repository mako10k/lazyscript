#pragma once

#include <stdio.h>
typedef struct lseappl lseappl_t;

#include "array.h"
#include "eenv.h"
#include "expr.h"
#include "tenv.h"
#include "thunk.h"

lseappl_t *lseappl(lsexpr_t *func);
void lseappl_push_arg(lseappl_t *eappl, lsexpr_t *arg);
void lseappl_push_args(lseappl_t *eappl, lsarray_t *args);
const lsexpr_t *lseappl_get_func(const lseappl_t *eappl);
unsigned int lseappl_get_argc(const lseappl_t *eappl);
lsexpr_t *lseappl_get_arg(const lseappl_t *eappl, unsigned int i);
void lseappl_print(FILE *stream, int prec, int indent, const lseappl_t *eappl);
int lseappl_prepare(lseappl_t *eappl, lseenv_t *eenv);
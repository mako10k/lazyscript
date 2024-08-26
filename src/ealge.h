#pragma once

#include <stdio.h>

typedef struct lsealge lsealge_t;

#include "array.h"
#include "eenv.h"
#include "expr.h"
#include "str.h"
#include "tenv.h"
#include "thunk.h"

lsealge_t *lsealge(const lsstr_t *constr);
void lsealge_push_arg(lsealge_t *ealge, lsexpr_t *arg);
void lsealge_push_args(lsealge_t *ealge, const lsarray_t *args);
const lsstr_t *lsealge_get_constr(const lsealge_t *ealge);
unsigned int lsealge_get_argc(const lsealge_t *ealge);
lsexpr_t *lsealge_get_arg(const lsealge_t *ealge, int i);
void lsealge_print(FILE *fp, int prec, int indent, const lsealge_t *ealge);
int lsealge_prepare(lsealge_t *ealge, lseenv_t *eenv);
lsthunk_t *lsealge_thunk(lstenv_t *tenv, const lsealge_t *ealge);
#pragma once

#include <stdio.h>

typedef struct lsealge lsealge_t;

#include "common/array.h"
#include "common/str.h"
#include "expr/eenv.h"
#include "expr/elist.h"
#include "expr/expr.h"
#include "lazyscript.h"

lsealge_t *lsealge_new(const lsstr_t *constr);
void lsealge_add_arg(lsealge_t *ealge, const lsexpr_t *arg);
void lsealge_concat_args(lsealge_t *ealge, const lselist_t *args);
const lsstr_t *lsealge_get_constr(const lsealge_t *ealge);
lssize_t lsealge_get_arg_count(const lsealge_t *ealge);
const lsexpr_t *lsealge_get_arg(const lsealge_t *ealge, int i);
const lselist_t *lsealge_get_args(const lsealge_t *ealge);
void lsealge_print(FILE *fp, lsprec_t prec, int indent, const lsealge_t *ealge);
lspres_t lsealge_prepare(const lsealge_t *ealge, lseenv_t *eenv);
#pragma once

#include <stdio.h>
typedef struct lseappl lseappl_t;

#include "common/array.h"
#include "expr/eenv.h"
#include "expr/elist.h"
#include "expr/expr.h"
#include "lazyscript.h"

lseappl_t *lseappl_new(lsexpr_t *func);
void lseappl_add_arg(lseappl_t *eappl, lsexpr_t *arg);
void lseappl_concat_args(lseappl_t *eappl, const lselist_t *args);
const lselist_t *lseappl_get_args(const lseappl_t *eappl);
const lsexpr_t *lseappl_get_func(const lseappl_t *eappl);
lssize_t lseappl_get_arg_count(const lseappl_t *eappl);
lsexpr_t *lseappl_get_arg(const lseappl_t *eappl, lssize_t i);
void lseappl_print(FILE *stream, lsprec_t prec, int indent,
                   const lseappl_t *eappl);
lspres_t lseappl_prepare(lseappl_t *eappl, lseenv_t *eenv);
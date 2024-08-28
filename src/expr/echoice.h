#pragma once

typedef struct lsechoice lsechoice_t;

#include "expr/expr.h"

lsechoice_t *lsechoice_new(lsexpr_t *left, lsexpr_t *right);
lsexpr_t *lsechoice_get_left(const lsechoice_t *echoice);
lsexpr_t *lsechoice_get_right(const lsechoice_t *echoice);
void lsechoice_print(FILE *fp, lsprec_t prec, int indent,
                     const lsechoice_t *echoice);
lspres_t lsechoice_prepare(lsechoice_t *echoice, lseenv_t *env);
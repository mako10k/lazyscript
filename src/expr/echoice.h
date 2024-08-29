#pragma once

typedef struct lsechoice lsechoice_t;

#include "expr/expr.h"

const lsechoice_t *lsechoice_new(const lsexpr_t *left, const lsexpr_t *right);
const lsexpr_t *lsechoice_get_left(const lsechoice_t *echoice);
const lsexpr_t *lsechoice_get_right(const lsechoice_t *echoice);
void lsechoice_print(FILE *fp, lsprec_t prec, int indent,
                     const lsechoice_t *echoice);
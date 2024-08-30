#pragma once

typedef struct lsechoice lsechoice_t;

#include "expr/expr.h"

#define lsapi_echoice_new lsapi_nn12 lsapi_wur

/**
 * Create a new choice expression.
 * @param left The left expression.
 * @param right The right expression.
 * @return The new choice expression.
 */
lsapi_echoice_new const lsechoice_t *lsechoice_new(const lsexpr_t *left,
                                                   const lsexpr_t *right);

/**
 * Get the left expression of a choice expression.
 * @param echoice The choice expression.
 * @return The left expression.
 */
lsapi_get const lsexpr_t *lsechoice_get_left(const lsechoice_t *echoice);

/**
 * Get the right expression of a choice expression.
 * @param echoice The choice expression.
 * @return The right expression.
 */
lsapi_get const lsexpr_t *lsechoice_get_right(const lsechoice_t *echoice);

/**
 * Print a choice expression.
 * @param fp The file to print to.
 * @param prec The precedence of the parent expression.
 * @param indent The indentation level.
 * @param echoice The choice expression.
 */
lsapi_print void lsechoice_print(FILE *fp, lsprec_t prec, int indent,
                                 const lsechoice_t *echoice);
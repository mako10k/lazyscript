#pragma once

#include <stdio.h>

/** Algebraic expression. */
typedef struct lsealge lsealge_t;

#include "common/str.h"
#include "expr/expr.h"

#define lsapi_ealge_new lsapi_nn1 lsapi_wur
#define lsapi_ealge_add_arg lsapi_nn12 lsapi_wur
#define lsapi_ealge_concat_args lsapi_nn13 lsapi_wur

/**
 * Create a new algebraic expression.
 * @param constr The constructor of the algebraic expression.
 * @return The new algebraic expression. (with no args)
 */
lsapi_ealge_new const lsealge_t *
lsealge_new(const lsstr_t *constr, lssize_t argc, const lsexpr_t *args[]);

/**
 * Add an argument to the algebraic expression.
 * @param ealge The algebraic expression.
 * @param arg The argument to add.
 * @return The algebraic expression with the argument added.
 */
lsapi_ealge_add_arg const lsealge_t *lsealge_add_arg(const lsealge_t *ealge,
                                                     const lsexpr_t *arg);

/**
 * Concatenate arguments to the algebraic expression.
 * @param ealge The algebraic expression.
 * @param args The arguments to concatenate.
 * @return The algebraic expression with the arguments concatenated.
 */
lsapi_ealge_concat_args const lsealge_t *
lsealge_concat_args(const lsealge_t *ealge, lssize_t argc,
                    const lsexpr_t *args[]);

/**
 * Get the constructor of the algebraic expression.
 * @param ealge The algebraic expression.
 * @return The constructor of the algebraic expression.
 */
lsapi_get const lsstr_t *lsealge_get_constr(const lsealge_t *ealge);

/**
 * Get the number of arguments of the algebraic expression.
 * @param ealge The algebraic expression.
 * @return The number of arguments of the algebraic expression.
 */
lsapi_get lssize_t lsealge_get_argc(const lsealge_t *ealge);

/**
 * Get the i-th argument of the algebraic expression.
 * @param ealge The algebraic expression.
 * @param i The index of the argument.
 * @return The i-th argument of the algebraic expression.
 */
lsapi_get const lsexpr_t *lsealge_get_arg(const lsealge_t *ealge, int i);

/**
 * Get the arguments of the algebraic expression.
 * @param ealge The algebraic expression.
 * @return The arguments of the algebraic expression.
 */
lsapi_get const lsexpr_t *const *lsealge_get_args(const lsealge_t *ealge);

/**
 * Print the algebraic expression.
 * @param fp The file to print to.
 * @param prec The precedence of the parent expression.
 * @param indent The indentation level.
 * @param ealge The algebraic expression to print.
 */
lsapi_print void lsealge_print(FILE *fp, lsprec_t prec, int indent,
                               const lsealge_t *ealge);
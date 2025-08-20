#pragma once

#include <stdio.h>
typedef struct lseappl lseappl_t;

#include "expr/expr.h"
#include "lazyscript.h"

#define lsapi_eappl_new lsapi_nn13 lsapi_wur
#define lsapi_eappl_add_arg lsapi_nn12 lsapi_wur
#define lsapi_eappl_concat_args lsapi_nn13 lsapi_wur

/**
 * Create a new application expression.
 * @param func The function to apply.
 * @param argc The number of arguments.
 * @param args The arguments.
 * @return The new application expression.
 */
lsapi_eappl_new const lseappl_t* lseappl_new(const lsexpr_t* func, size_t argc,
                                             const lsexpr_t* args[]);

/**
 * Add an argument to an application expression.
 * @param eappl The application expression.
 * @param arg The argument to add.
 * @return The new application expression.
 */
lsapi_eappl_add_arg const lseappl_t* lseappl_add_arg(const lseappl_t* eappl, const lsexpr_t* arg);

/**
 * Concatenate arguments to an application expression.
 * @param eappl The application expression.
 * @param argc The number of arguments.
 * @param args The arguments.
 * @return The new application expression.
 */
lsapi_eappl_concat_args const lseappl_t* lseappl_concat_args(lseappl_t* eappl, size_t argc,
                                                             const lsexpr_t* args[]);

/**
 * Get the number of arguments of an application expression.
 * @param eappl The application expression.
 * @return The number of arguments.
 */
lsapi_get lssize_t lseappl_get_argc(const lseappl_t* eappl);

/**
 * Get the arguments of an application expression.
 * @param eappl The application expression.
 * @return The arguments.
 */
lsapi_get const lsexpr_t* const* lseappl_get_args(const lseappl_t* eappl);

/**
 * Get the number of arguments of an application expression.
 * @param eappl The application expression.
 * @return The number of arguments.
 */
lsapi_get const lsexpr_t* lseappl_get_func(const lseappl_t* eappl);

/**
 * Print an application expression.
 * @param stream The output stream.
 * @param prec The precedence of the parent expression.
 * @param indent The indentation level.
 * @param eappl The application expression.
 */
lsapi_print void lseappl_print(FILE* stream, lsprec_t prec, int indent, const lseappl_t* eappl);
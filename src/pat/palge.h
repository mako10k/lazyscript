#pragma once

#include "lstypes.h"
typedef struct lspalge lspalge_t;

#include "common/array.h"
#include "common/str.h"
#include "lazyscript.h"
#include "pat/pat.h"

#define lsapi_palge_new lsapi_nn13 lsapi_wur
#define lsapi_palge_add_arg lsapi_nn12 lsapi_wur
#define lsapi_palge_concat_args lsapi_nn13 lsapi_wur

/**
 * Create a new pattern algebra.
 *
 * @param constr the constructor of the algebra
 * @param argc the number of arguments
 * @param args the arguments
 * @return the new algebra
 */
lsapi_palge_new const lspalge_t *
lspalge_new(const lsstr_t *constr, lssize_t argc, const lspat_t *args[]);

/**
 * Add an argument to a pattern algebra.
 *
 * @param palge the algebra
 * @param arg the argument
 * @return the algebra with the argument added
 */
lsapi_palge_add_arg const lspalge_t *lspalge_add_arg(const lspalge_t *palge,
                                                     const lspat_t *arg);

/**
 * Concatenate arguments to a pattern algebra.
 *
 * @param palge the algebra
 * @param argc the number of arguments
 * @param args the arguments
 * @return the algebra with the arguments concatenated
 */
lsapi_palge_concat_args const lspalge_t *
lspalge_concat_args(const lspalge_t *palge, lssize_t argc,
                    const lspat_t *args[]);

/**
 * Get the constructor of a pattern algebra.
 *
 * @param palge the algebra
 * @return the constructor
 */
lsapi_get const lsstr_t *lspalge_get_constr(const lspalge_t *palge);

/**
 * Get the number of arguments of a pattern algebra.
 *
 * @param palge the algebra
 * @return the number of arguments
 */
lsapi_get lssize_t lspalge_get_argc(const lspalge_t *palge);

/**
 * Get an argument of a pattern algebra.
 *
 * @param palge the algebra
 * @param i the index of the argument
 * @return the argument
 */
lsapi_get const lspat_t *lspalge_get_arg(const lspalge_t *palge, int i);

/**
 * Get the arguments of a pattern algebra.
 *
 * @param palge the algebra
 * @return the arguments
 */
lsapi_get const lspat_t *const *lspalge_get_args(const lspalge_t *palge);

/**
 * Print a pattern algebra.
 *
 * @param fp the file to print to
 * @param prec the precedence of the context
 * @param indent the indentation level
 * @param palge the algebra
 */
lsapi_print void lspalge_print(FILE *fp, int prec, int indent,
                               const lspalge_t *palge);

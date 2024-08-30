#pragma once

typedef struct lseclosure lseclosure_t;

#include "expr/expr.h"
#include "misc/bind.h"

#define lsapi_eclosure_new lsapi_nn13 lsapi_wur

/**
 * Create a new expression closure.
 * @param expr The expression.
 * @param bindc The number of binds.
 * @param binds The binds.
 * @return The new expression closure.
 */
lsapi_eclosure_new const lseclosure_t *
lseclosure_new(const lsexpr_t *expr, size_t bindc, const lsbind_t *binds[]);

/**
 * Print an expression closure.
 * @param stream The file to print to.
 * @param prec The precedence of the parent expression.
 * @param indent The indentation level.
 * @param eclosure The expression closure.
 */
lsapi_print void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                                  const lseclosure_t *eclosure);
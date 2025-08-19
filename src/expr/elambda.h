#pragma once

typedef struct lselambda lselambda_t;

#include "expr/expr.h"
#include "pat/pat.h"
#include <stdio.h>

#define lsapi_elambda_new lsapi_nn12 lsapi_wur

/**
 * Create a new lambda expression.
 * @param arg The argument pattern.
 * @param body The body expression.
 * @return The new lambda expression.
 */
lsapi_elambda_new const lselambda_t* lselambda_new(const lspat_t* arg, const lsexpr_t* body);

/**
 * Get the argument pattern of a lambda expression.
 * @param elambda The lambda expression.
 * @return The argument pattern.
 */
lsapi_get const lspat_t* lselambda_get_param(const lselambda_t* elambda);

/**
 * Get the body of a lambda expression.
 * @param elambda The lambda expression.
 * @return The body.
 */
lsapi_get const lsexpr_t* lselambda_get_body(const lselambda_t* elambda);

/**
 * Print a lambda expression.
 * @param fp The file to print to.
 * @param prec The precedence of the parent expression.
 * @param indent The indentation level.
 * @param elambda The lambda expression.
 */
lsapi_print void lselambda_print(FILE* fp, lsprec_t prec, int indent, const lselambda_t* elambda);
#pragma once

/** A list expression. */
typedef struct lselist lselist_t;

#include "expr/expr.h"

#define lsapi_elist_new lsapi_wur __attribute__((const))
#define lsapi_elist_op1 lsapi_nn12 lsapi_wur
#define lsapi_elist_op2 lsapi_nn12 lsapi_wur

/**
 * Create a new list expression.
 * @return The new list expression.
 */
lsapi_elist_new const lselist_t *lselist_new(void);

/**
 * Push an expression onto a list expression.
 * @param elist The list expression.
 * @param expr The expression to push.
 * @return The new list expression.
 */
lsapi_elist_op1 const lselist_t *lselist_push(const lselist_t *elist,
                                              const lsexpr_t *expr);

/**
 * Pop an expression from a list expression.
 * @param elist The list expression.
 * @param pexpr The popped expression.
 * @return The new list expression.
 */
lsapi_elist_op2 const lselist_t *lselist_pop(const lselist_t *elist,
                                             const lsexpr_t **pexpr);

/**
 * Unshift an expression onto a list expression.
 * @param elist The list expression.
 * @param expr The expression to unshift.
 * @return The new list expression.
 */
lsapi_elist_op1 const lselist_t *lselist_unshift(const lselist_t *elist,
                                                 const lsexpr_t *expr);

/**
 * Shift an expression from a list expression.
 * @param elist The list expression.
 * @param pexpr The shifted expression.
 * @return The new list expression.
 */
lsapi_elist_op2 const lselist_t *lselist_shift(const lselist_t *elist,
                                               const lsexpr_t **pexpr);

/**
 * Get the number of expressions in a list expression.
 * @param elist The list expression.
 * @return The number of expressions.
 */
lsapi_get lssize_t lselist_count(const lselist_t *elist);

/**
 * Get the i-th expression of a list expression.
 * @param elist The list expression.
 * @param i The index of the expression.
 * @return The i-th expression.
 */
lsapi_get const lsexpr_t *lselist_get(const lselist_t *elist, lssize_t i);

/**
 * Concatenate two list expressions.
 * @param elist1 The first list expression.
 * @param elist2 The second list expression.
 * @return The concatenated list expression.
 */
lsapi_elist_op1 const lselist_t *lselist_concat(const lselist_t *elist1,
                                                const lselist_t *elist2);

/**
 * Get the next list expression.
 * @param elist The list expression.
 * @return The next list expression.
 */
lsapi_get const lselist_t *lselist_get_next(const lselist_t *elist);

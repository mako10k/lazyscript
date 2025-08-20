#pragma once

/** Integer type */
typedef struct lsint lsint_t;

#include "lstypes.h"
#include <stdio.h>

/**
 * Make a new integer.
 * @param val Value.
 * @return New integer.
 */
const lsint_t* lsint_new(int val);

/**
 * Print an integer.
 * @param fp File pointer.
 * @param prec Precedence.
 * @param indent Indentation.
 * @param val Integer.
 */
void lsint_print(FILE* fp, lsprec_t prec, int indent, const lsint_t* val);

/**
 * Compare two integers.
 * @param val1 First integer.
 * @param val2 Second integer.
 * @return 1 if equal, 0 otherwise.
 */
int lsint_eq(const lsint_t* val1, const lsint_t* val2);

/**
 * Get the integer as a C int (NULL means 0).
 */
int lsint_get(const lsint_t* val);

/**
 * Add two integers.
 * @param val1 First integer.
 * @param val2 Second integer.
 * @return Sum.
 */
const lsint_t* lsint_add(const lsint_t* val1, const lsint_t* val2);

/**
 * Subtract two integers.
 * @param val1 First integer.
 * @param val2 Second integer.
 * @return Sum.
 */
const lsint_t* lsint_sub(const lsint_t* val1, const lsint_t* val2);
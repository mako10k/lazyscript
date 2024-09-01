#pragma once

/** array type */
typedef struct lsarray lsarray_t;

/** array data type */
typedef const void *lsarray_data_t;

#include "lstypes.h"

// *******************************
// Array.
// *******************************
/**
 * Create a new array.
 * @param size Size.
 * @param values Values.
 */
const lsarray_t *lsarray_new(lssize_t size, lsarray_data_t const *values);

/**
 * Get a value from an array.
 * @param ary Array.
 * @return C array of value.
 */
lsarray_data_t const *lsarray_get(const lsarray_t *ary);

/**
 * Get the size of an array.
 * @param ary Array.
 * @return Size.
 */
lssize_t lsarray_get_size(const lsarray_t *ary);

/**
 * Push a value into an array.
 * @param ary Array.
 * @param val Value.
 * @return New array.
 */
const lsarray_t *lsarray_push(const lsarray_t *ary, lsarray_data_t val);

/**
 * Pop a value from an array.
 * @param ary Array.
 * @param pnew_ary New array.
 * @return Value.
 */
lsarray_data_t lsarray_pop(const lsarray_t *ary, const lsarray_t **pnew_ary);

/**
 * Unshift a value into an array.
 * @param ary Array.
 * @param val Value.
 * @return New array.
 */
const lsarray_t *lsarray_unshift(const lsarray_t *ary, lsarray_data_t val);

/**
 * Shift a value from an array.
 * @param ary Array.
 * @param pnew_ary New array.
 * @return Value.
 */
lsarray_data_t lsarray_shift(const lsarray_t *ary, const lsarray_t **pnew_ary);

/**
 * Concatenate two arrays.
 * @param ary1 First array.
 * @param ary2 Second array.
 * @return Concatenated array.
 */
const lsarray_t *lsarray_concat(const lsarray_t *ary1, const lsarray_t *ary2);

#pragma once

/** array type */
typedef struct lsarray lsarray_t;

/** array data type */
typedef void *lsarray_data_t;

#include "lstypes.h"

// *******************************
// Array.
// *******************************
/**
 * Create a new array.
 * @return a new array.
 */
lsarray_t *lsarray_new(void);

/**
 * Free an array.
 * @param ary Array.
 */
void lsarray_free(lsarray_t *ary);

/**
 * Set a value in an array.
 * @param ary Array.
 * @param i Index.
 * @param val Value.
 */
void lsarray_set(lsarray_t *ary, lssize_t i, lsarray_data_t val);

/**
 * Get a value from an array.
 * @param ary Array.
 * @param i Index.
 * @return Value.
 */
lsarray_data_t lsarray_get(const lsarray_t *ary, lssize_t i);

/**
 * Get the size of an array.
 * @param ary Array.
 * @return Size.
 */
lssize_t lsarray_get_size(const lsarray_t *ary);

/**
 * Set the size of an array.
 * @param ary Array.
 * @param new_size New size.
 */
void lsarray_set_size(lsarray_t *ary, lssize_t new_size);

/**
 * Push a value into an array.
 * @param ary Array.
 * @param val Value.
 */
void lsarray_push(lsarray_t *ary, lsarray_data_t val);

/**
 * Pop a value from an array.
 * @param ary Array.
 * @return Value.
 */
lsarray_data_t lsarray_pop(lsarray_t *ary);

/**
 * Unshift a value into an array.
 * @param ary Array.
 * @param val Value.
 */
void lsarray_unshift(lsarray_t *ary, lsarray_data_t val);

/**
 * Shift a value from an array.
 * @param ary Array.
 * @return Value.
 */
lsarray_data_t lsarray_shift(lsarray_t *ary);

/**
 * Clone an array.
 * @param ary Array.
 * @return Cloned array.
 */
lsarray_t *lsarray_clone(const lsarray_t *ary);

/**
 * Concatenate two arrays.
 * @param ary1 First array.
 * @param ary2 Second array.
 * @return Concatenated array. Same as ary1.
 */
lsarray_t *lsarray_concat(lsarray_t *ary1, const lsarray_t *ary2);

/**
 * Concatenate two arrays and clone the result.
 * @param ary1 First array.
 * @param ary2 Second array.
 * @return Cloned concatenated array.
 */
lsarray_t *lsarray_concat_clone(const lsarray_t *ary1, const lsarray_t *ary2);
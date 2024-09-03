#pragma once

#include <stdarg.h>

/** array type */
typedef struct lsarray lsarray_t;

#include "lstypes.h"

const void *const *lsa_newv(lssize_t size, va_list ap);
const void *const *lsa_new(lssize_t size, ...);
const void *const *lsa_newa(lssize_t size, const void *const *ary);
const void *const *lsa_push(lssize_t size, const void *const *ary,
                            const void *val);
const void *lsa_pop(lssize_t size, const void *const *ary,
                    const void *const **pary);
const void *const *lsa_unshift(lssize_t size, const void *const *ary,
                               const void *val);
const void *lsa_shift(lssize_t size, const void *const *ary,
                      const void *const **pary);
const void *const *lsa_concatv(lssize_t size1, const void *const *ary1,
                               lssize_t size2, va_list ap);
const void *const *lsa_concat(lssize_t size1, const void *const *ary1,
                              lssize_t size2, ...);
const void *const *lsa_concata(lssize_t size1, const void *const *ary1,
                               lssize_t size2, const void *const *ary2);
const void *const *lsa_splicev(lssize_t size, const void *const *ary,
                               lssize_t start, lssize_t end, lssize_t insize,
                               va_list ap);
const void *const *lsa_splicea(lssize_t size, const void *const *ary,
                               lssize_t start, lssize_t end, lssize_t insize,
                               const void *const *ins);
const void *const *lsa_splice(lssize_t size, const void *const *ary,
                              lssize_t start, lssize_t end, lssize_t insize,
                              ...);

// *******************************
// Array.
// *******************************
/**
 * Create a new array.
 * @param size Size.
 * @param ap Values.
 * @return New array.
 */
const lsarray_t *lsarray_newv(lssize_t size, va_list ap);
/**
 * Create a new array.
 * @param size Size.
 * @param values Values.
 * @return New array.
 */
const lsarray_t *lsarray_newa(lssize_t size, const void *const *values);

/**
 * Create a new array.
 * @param size Size.
 * @param ... Values.
 * @return New array.
 */
const lsarray_t *lsarray_new(lssize_t size, ...);

/**
 * Get a value from an array.
 * @param ary Array.
 * @return C array of value.
 */
const void *const *lsarray_get(const lsarray_t *ary);

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
const lsarray_t *lsarray_push(const lsarray_t *ary, const void *val);

/**
 * Pop a value from an array.
 * @param ary Array.
 * @param pnew_ary New array.
 * @return Value.
 */
const void *lsarray_pop(const lsarray_t *ary, const lsarray_t **pnew_ary);

/**
 * Unshift a value into an array.
 * @param ary Array.
 * @param val Value.
 * @return New array.
 */
const lsarray_t *lsarray_unshift(const lsarray_t *ary, const void *val);

/**
 * Shift a value from an array.
 * @param ary Array.
 * @param pnew_ary New array.
 * @return Value.
 */
const void *lsarray_shift(const lsarray_t *ary, const lsarray_t **pnew_ary);

/**
 * Concatenate two arrays.
 * @param ary1 First array.
 * @param ary2 Second array.
 * @return Concatenated array.
 */
const lsarray_t *lsarray_concat(const lsarray_t *ary1, const lsarray_t *ary2);

/**
 * Splice an array.
 * @param ary Array.
 * @param start Start index.
 * @param end End index.
 * @param size Size to insert
 * @param ap Values to insert.
 * @return Spliced array.
 */
const lsarray_t *lsarray_splicev(const lsarray_t *ary, lssize_t start,
                                 lssize_t end, lssize_t size, va_list ap);

/**
 * Splice an array.
 * @param ary Array.
 * @param start Start index.
 * @param end End index.
 * @param ary2 Values to insert.
 * @return Spliced array.
 */
const lsarray_t *lsarray_splicea(const lsarray_t *ary, lssize_t start,
                                 lssize_t end, const lsarray_t *ary2);
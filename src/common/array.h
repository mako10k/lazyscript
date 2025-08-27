#pragma once

#include <stdarg.h>

/** array type */
typedef struct lsarray lsarray_t;

#include "lstypes.h"

#define lsapi_carray lsapi_wur

// *******************************
// C-array.
// *******************************

/**
 * Create a new C-array.
 * @param size Size.
 * @param ... Values.
 * @return New C-array. (it may be NULL, if size is 0)
 */
lsapi_carray const void* const* lsa_new(lssize_t size, ...);

/**
 * Create a new C-array.
 * @param size Size.
 * @param ap Values.
 * @return New C-array. (it may be NULL, when size is 0)
 */
lsapi_carray const void* const* lsa_newv(lssize_t size, va_list ap);

/**
 * Create a new C-array.
 * @param size Size.
 * @param ary Values.
 * @return New C-array. (it may be NULL, when size is 0)
 */
// removed: lsa_newa (unused)

/**
 * Create a new C-array with a value appended at the end.
 * @param size Size.
 * @param ary C-array. (it can be NULL, when size is 0)
 * @param val Value.
 * @return New C-array.
 */
const void* const* lsa_push(lssize_t size, const void* const* ary, const void* val);

/**
 * Create a new C-array with a value without the last element.
 * @param size Size. (must be greater than 0)
 * @param ary C-array. (it must not be NULL)
 * @param pval Pointer to returning the last value.
 * when size is 1)
 * @return New C-array. (it may be NULL, when size is 1)
 */
// removed: lsa_pop (unused)

/**
 * Create a new C-array with a value prepended at the beginning.
 * @param size Size.
 * @param ary C-array. (it can be NULL, when size is 0)
 * @param val Value.
 * @return New C-array.
 */
// removed: lsa_unshift (unused)

/**
 * Create a new C-array without the first element.
 * @param size Size. (must be greater than 0)
 * @param ary C-array. (it must not be NULL)
 * @param pval Pointer to returning the first value.
 * @return Value. (it may be NULL, when size is 1)
 */
const void* const* lsa_shift(lssize_t size, const void* const* ary, const void** pval);

/**
 * Return the concatenated C-array.
 * @param size1 Size of the first C-array.
 * @param ary1 The first C-array.
 * @param size2 Size of the second values.
 * @param ap Second Values.
 * @return Concatenated C-array.
 */
// removed: lsa_concatv (unused)

/**
 * Return the concatenated C-array.
 * @param size1 Size of the first C-array.
 * @param ary1 The first C-array.
 * @param size2 Size of the second values.
 * @param ... Second Values
 * @return Concatenated C-array.
 */
// removed: lsa_concat (unused)

/**
 * Return the concatenated C-array.
 * @param size1 Size of the first C-array.
 * @param ary1 The first C-array.
 * @param size2 Size of the second C-array.
 * @param ary2 The second C-array.
 * @return Concatenated C-array.
 */
const void* const* lsa_concata(lssize_t size1, const void* const* ary1, lssize_t size2,
                               const void* const* ary2);

/**
 * Return the sliced C-array.
 * @param size Size of the C-array.
 * @param ary The C-array.
 * @param start Start index.
 * @param end End index.
 * @return Sliced C-array.
 */
// removed: lsa_slice (unused)

/**
 * Return the spliced C-array.
 * @param size Size of the C-array.
 * @param ary The C-array.
 * @param start Start index.
 * @param end End index.
 * @param insize Size of the inserted values.
 * @param ap Inserted values.
 * @return Spliced C-array.
 */
// removed: lsa_splicev (unused)

/**
 * Return the spliced C-array.
 * @param size Size of the C-array.
 * @param ary The C-array.
 * @param start Start index.
 * @param end End index.
 * @param insize Size of the inserted values.
 * @param ins Inserted values.
 * @return Spliced C-array.
 */
// removed: lsa_splicea (unused)

/**
 * Return the spliced C-array.
 * @param size Size of the C-array.
 * @param ary The C-array.
 * @param start Start index.
 * @param end End index.
 * @param insize Size of the inserted values.
 * @param ... Inserted values.
 * @return Spliced C-array.
 */
// removed: lsa_splice (unused)

// *******************************
// Array.
// *******************************
/**
 * Create a new array.
 * @param size Size.
 * @param ... Values.
 * @return New array.
 */
const lsarray_t* lsarray_new(lssize_t size, ...);

/**
 * Create a new array.
 * @param size Size.
 * @param values Values.
 * @return New array.
 */
// removed: lsarray_newa (unused)

/**
 * Create a new array.
 * @param size Size.
 * @param ap Values.
 * @return New array.
 */
const lsarray_t* lsarray_newv(lssize_t size, va_list ap);

/**
 * Get a value from an array.
 * @param ary Array.
 * @return C array of value.
 */
const void* const* lsarray_get(const lsarray_t* ary);

/**
 * Get the size of an array.
 * @param ary Array.
 * @return Size.
 */
lssize_t lsarray_get_size(const lsarray_t* ary);

/**
 * Push a value into an array.
 * @param ary Array.
 * @param val Value.
 * @return New array.
 */
const lsarray_t* lsarray_push(const lsarray_t* ary, const void* val);

/**
 * Pop a value from an array.
 * @param ary Array.
 * @param pval Pointer to returning the value.
 * @return New array.
 */
// removed: lsarray_pop (unused)

/**
 * Unshift a value into an array.
 * @param ary Array.
 * @param val Value.
 * @return New array.
 */
// removed: lsarray_unshift (unused)

/**
 * Shift a value from an array.
 * @param ary Array.
 * @param pval Pointer to returning the value.
 * @return New array.
 */
// removed: lsarray_shift (unused)

/**
 * Concatenate two arrays.
 * @param ary1 First array.
 * @param ary2 Second array.
 * @return Concatenated array.
 */
// removed: lsarray_concat (unused)

/**
 * Splice an array.
 * @param ary Array.
 * @param start Start index.
 * @param end End index.
 * @param insize Size to insert
 * @param ap Values to insert.
 * @return Spliced array.
 */
// removed: lsarray_splicev (unused)

/**
 * Splice an array.
 * @param ary Array.
 * @param start Start index.
 * @param end End index.
 * @param ins Values to insert.
 * @return Spliced array.
 */
// removed: lsarray_splicea (unused)

/**
 * Splice an array.
 * @param ary Array.
 * @param start Start index.
 * @param end End index.
 * @param insize Size to insert
 * @param ... Values to insert.
 * @return Spliced array.
 */
// removed: lsarray_splice (unused)
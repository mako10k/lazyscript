#pragma once

/** list type */
typedef struct lslist lslist_t;

/** list data type */
typedef const void* lslist_data_t;

#include "lstypes.h"

// *******************************
// List.
// *******************************

/**
 * Create a new list.
 * @return a new list.
 */
const lslist_t* lslist_new(void);

/**
 * Push a value into a list.
 * @param list List.
 * @param data Value.
 * @return New list.
 */
const lslist_t* lslist_push(const lslist_t* list, lslist_data_t data);

/**
 * Pop a value from a list.
 * @param list List.
 * @param pdata Value.
 * @return New list.
 */
const lslist_t* lslist_pop(const lslist_t* list, lslist_data_t* pdata);

/**
 * Unshift a value into a list.
 * @param list List.
 * @param data Value.
 * @return New list.
 */
const lslist_t* lslist_unshift(const lslist_t* list, lslist_data_t data);

/**
 * Shift a value from a list.
 * @param list List.
 * @param pdata Value.
 * @return New list.
 */
const lslist_t* lslist_shift(const lslist_t* list, lslist_data_t* pdata);

/**
 * Get the count of a list.
 * @param list List.
 * @return Count.
 */
lssize_t lslist_count(const lslist_t* list);

/**
 * Get a value from a list.
 * @param list List.
 * @param i Index.
 * @return Value.
 */
lslist_data_t lslist_get(const lslist_t* list, lssize_t i);

/**
 * Concatenate two lists.
 * @param list1 First list.
 * @param list2 Second list.
 * @return New list.
 */
const lslist_t* lslist_concat(const lslist_t* list1, const lslist_t* list2);

/**
 * Get the next list.
 * @param list List.
 * @return Next list.
 */
const lslist_t* lslist_get_next(const lslist_t* list);

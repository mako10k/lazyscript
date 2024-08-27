#pragma once

typedef struct lsarray lsarray_t;
typedef void *lsarray_data_t;

#include "lstypes.h"

// *******************************
// Array.
// *******************************
lsarray_t *lsarray_new(void);

void lsarray_free(lsarray_t *ary);

void lsarray_set(lsarray_t *ary, lssize_t i, lsarray_data_t val);

lsarray_data_t lsarray_get(const lsarray_t *ary, lssize_t i);

lssize_t lsarray_get_size(const lsarray_t *ary);

void lsarray_set_size(lsarray_t *ary, lssize_t new_size);

void lsarray_push(lsarray_t *ary, lsarray_data_t val);

lsarray_data_t lsarray_pop(lsarray_t *ary);

void lsarray_unshift(lsarray_t *ary, lsarray_data_t val);

lsarray_data_t lsarray_shift(lsarray_t *ary);

lsarray_t *lsarray_clone(const lsarray_t *ary);

lsarray_t *lsarray_concat(lsarray_t *ary1, const lsarray_t *ary2);

lsarray_t *lsarray_concat_clone(const lsarray_t *ary1, const lsarray_t *ary2);
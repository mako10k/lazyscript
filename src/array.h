#pragma once

typedef struct lsarray lsarray_t;

// *******************************
// Array.
// *******************************
lsarray_t *lsarray(void);

void lsarray_free(lsarray_t *array);

void lsarray_set(lsarray_t *array, unsigned int index, void *value);

void *lsarray_get(const lsarray_t *array, unsigned int index);

unsigned int lsarray_get_size(const lsarray_t *array);

void lsarray_set_size(lsarray_t *array, unsigned int size);

void lsarray_push(lsarray_t *array, void *value);

void *lsarray_pop(lsarray_t *array);

void lsarray_unshift(lsarray_t *array, void *value);

void *lsarray_shift(lsarray_t *array);

lsarray_t *lsarray_clone(const lsarray_t *array);

lsarray_t *lsarray_concat(lsarray_t *array1, const lsarray_t *array2);
#pragma once

typedef struct lsarray lsarray_t;

// *******************************
// Array.
// *******************************
lsarray_t *lsarray(void);

void lsarray_free(lsarray_t *ary);

void lsarray_set(lsarray_t *ary, unsigned int i, void *val);

void *lsarray_get(const lsarray_t *ary, unsigned int i);

unsigned int lsarray_get_size(const lsarray_t *ary);

void lsarray_set_size(lsarray_t *ary, unsigned int new_size);

void lsarray_push(lsarray_t *ary, void *val);

void *lsarray_pop(lsarray_t *ary);

void lsarray_unshift(lsarray_t *ary, void *val);

void *lsarray_shift(lsarray_t *ary);

lsarray_t *lsarray_clone(const lsarray_t *ary);

lsarray_t *lsarray_concat(lsarray_t *ary1, const lsarray_t *ary2);
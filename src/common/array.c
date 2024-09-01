#include "common/array.h"
#include "common/malloc.h"
#include "lstypes.h"
#include <assert.h>
#include <stdio.h>

struct lsarray {
  lsarray_data_t *la_values;
  lssize_t la_size;
};

const lsarray_t *lsarray_new(lssize_t size, lsarray_data_t const *values) {
  assert(size == 0 || values != NULL);
  if (size == 0)
    return NULL;
  lsarray_t *ary = lsmalloc(sizeof(lsarray_t));
  ary->la_values = lsmalloc(size * sizeof(lsarray_data_t));
  for (lssize_t i = 0; i < size; i++)
    ary->la_values[i] = values[i];
  ary->la_size = size;
  return ary;
}

lsarray_data_t const *lsarray_get(const lsarray_t *ary) {
  if (ary == NULL)
    return NULL;
  return ary->la_values;
}

lssize_t lsarray_get_size(const lsarray_t *ary) {
  return ary == NULL ? 0 : ary->la_size;
}

const lsarray_t *lsarray_push(const lsarray_t *ary, lsarray_data_t val) {
  if (ary == NULL || ary->la_size == 0)
    return lsarray_new(1, &val);
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  lssize_t new_size = ary == NULL ? 1 : ary->la_size + 1;
  new_ary->la_values = lsmalloc(new_size * sizeof(lsarray_data_t));
  for (lssize_t i = 0; i < new_size - 1; i++)
    new_ary->la_values[i] = ary->la_values[i];
  new_ary->la_values[new_size - 1] = val;
  new_ary->la_size = new_size;
  return new_ary;
}

lsarray_data_t lsarray_pop(const lsarray_t *ary, const lsarray_t **pnew_ary) {
  assert(ary != NULL && ary->la_size > 0);
  if (pnew_ary == NULL)
    return ary->la_values[ary->la_size - 1];
  if (ary->la_size == 1) {
    *pnew_ary = NULL;
    return ary->la_values[0];
  }
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = ary->la_values;
  new_ary->la_size = ary->la_size - 1;
  *pnew_ary = new_ary;
  return ary->la_values[ary->la_size - 1];
}

const lsarray_t *lsarray_unshift(const lsarray_t *ary, lsarray_data_t val) {
  if (ary == NULL || ary->la_size == 0)
    return lsarray_new(1, &val);
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = lsmalloc((ary->la_size + 1) * sizeof(lsarray_data_t));
  new_ary->la_values[0] = val;
  for (lssize_t i = 0; i < ary->la_size; i++)
    new_ary->la_values[i + 1] = ary->la_values[i];
  new_ary->la_size = ary->la_size + 1;
  return new_ary;
}

lsarray_data_t lsarray_shift(const lsarray_t *ary, const lsarray_t **pnew_ary) {
  assert(ary != NULL && ary->la_size > 0);
  if (pnew_ary == NULL)
    return ary->la_values[0];
  if (ary->la_size == 1) {
    *pnew_ary = NULL;
    return ary->la_values[0];
  }
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = lsmalloc((ary->la_size - 1) * sizeof(lsarray_data_t));
  for (lssize_t i = 0; i < ary->la_size - 1; i++)
    new_ary->la_values[i] = ary->la_values[i + 1];
  new_ary->la_size = ary->la_size - 1;
  *pnew_ary = new_ary;
  return ary->la_values[0];
}

const lsarray_t *lsarray_concat(const lsarray_t *ary1, const lsarray_t *ary2) {
  if (ary1 == NULL)
    return ary2;
  if (ary2 == NULL)
    return ary1;
  lsarray_t *ary = lsmalloc(sizeof(lsarray_t));
  ary->la_values =
      lsmalloc((ary1->la_size + ary2->la_size) * sizeof(lsarray_data_t));
  ary->la_size = ary1->la_size + ary2->la_size;
  for (lssize_t i = 0; i < ary1->la_size; i++)
    ary->la_values[i] = ary1->la_values[i];
  for (lssize_t i = 0; i < ary2->la_size; i++)
    ary->la_values[ary1->la_size + i] = ary2->la_values[i];
  return ary;
}
#include "array.h"
#include "malloc.h"
#include <assert.h>
#include <stdio.h>

struct lsarray {
  lsarray_data_t *la_values;
  lssize_t la_size;
};

static void lsarray_resize(lsarray_t *ary, lssize_t new_size) {
  assert(ary != NULL);
  if (new_size != ary->la_size) {
    ary->la_values =
        lsrealloc(ary->la_values, new_size * sizeof(lsarray_data_t));
    for (lssize_t i = ary->la_size; i < new_size; i++)
      ary->la_values[i] = NULL;
  }
  ary->la_size = new_size;
}

lsarray_t *lsarray_new() {
  lsarray_t *array = lsmalloc(sizeof(lsarray_t));
  array->la_values = NULL;
  array->la_size = 0;
  return array;
}

void lsarray_free(lsarray_t *ary) {
  if (ary == NULL)
    return;
  if (ary->la_values != NULL)
    lsfree(ary->la_values);
  lsfree(ary);
}

void lsarray_set(lsarray_t *ary, lssize_t i, lsarray_data_t val) {
  assert(ary != NULL);
  if (i >= ary->la_size) {
    lssize_t new_size = i + 1;
    lsarray_resize(ary, new_size);
    ary->la_size = new_size;
  }
  ary->la_values[i] = val;
}

lsarray_data_t lsarray_get(const lsarray_t *ary, lssize_t i) {
  if (ary == NULL)
    return NULL;
  if (i >= ary->la_size)
    return NULL;
  return ary->la_values[i];
}

lssize_t lsarray_get_size(const lsarray_t *ary) {
  return ary == NULL ? 0 : ary->la_size;
}

void lsarray_set_size(lsarray_t *ary, lssize_t new_size) {
  assert(ary != NULL);
  lsarray_resize(ary, new_size);
  ary->la_size = new_size;
}

void lsarray_push(lsarray_t *ary, lsarray_data_t val) {
  assert(ary != NULL);
  lsarray_set(ary, ary->la_size, val);
}

lsarray_data_t lsarray_pop(lsarray_t *ary) {
  assert(ary != NULL);
  if (ary->la_size == 0)
    return NULL;
  lsarray_data_t value = lsarray_get(ary, ary->la_size - 1);
  lsarray_set_size(ary, ary->la_size - 1);
  return value;
}

void lsarray_unshift(lsarray_t *ary, lsarray_data_t val) {
  assert(ary != NULL);
  if (ary->la_size == 0) {
    lsarray_push(ary, val);
    return;
  }
  lsarray_set_size(ary, ary->la_size + 1);
  for (lssize_t i = ary->la_size - 1; i > 0; i--)
    ary->la_values[i] = ary->la_values[i - 1];
  ary->la_values[0] = val;
}

lsarray_data_t lsarray_shift(lsarray_t *ary) {
  assert(ary != NULL);
  if (ary->la_size == 0)
    return NULL;
  lsarray_data_t value = lsarray_get(ary, 0);
  for (lssize_t i = 1; i < ary->la_size; i++)
    ary->la_values[i - 1] = ary->la_values[i];
  lsarray_set_size(ary, ary->la_size - 1);
  return value;
}

lsarray_t *lsarray_clone(const lsarray_t *ary) {
  if (ary == NULL)
    return NULL;
  lsarray_t *clone = lsmalloc(sizeof(lsarray_t));
  clone->la_values = lsmalloc(ary->la_size * sizeof(lsarray_data_t));
  clone->la_size = ary->la_size;
  for (lssize_t i = 0; i < ary->la_size; i++)
    clone->la_values[i] = ary->la_values[i];
  return clone;
}

lsarray_t *lsarray_concat(lsarray_t *ary1, const lsarray_t *ary2) {
  if (ary2 == NULL)
    return ary1;
  if (ary1 == NULL)
    return lsarray_clone(ary2);
  ary1->la_values = lsrealloc(ary1->la_values, (ary1->la_size + ary2->la_size) *
                                                   sizeof(lsarray_data_t));
  ary1->la_size += ary2->la_size;
  for (lssize_t i = 0; i < ary2->la_size; i++)
    ary1->la_values[ary1->la_size + i] = ary2->la_values[i];
  ary1->la_size += ary2->la_size;
  return ary1;
}

lsarray_t *lsarray_concat_clone(const lsarray_t *ary1, const lsarray_t *ary2) {
  if (ary1 == NULL)
    return lsarray_clone(ary2);
  if (ary2 == NULL)
    return lsarray_clone(ary1);
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
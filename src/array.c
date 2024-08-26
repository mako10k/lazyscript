#include "array.h"
#include "malloc.h"
#include <assert.h>
#include <stdio.h>

struct lsarray {
  void **ka_values;
  unsigned int ka_size;
};

static void lsarray_resize(lsarray_t *ary, unsigned int new_size) {
  assert(ary != NULL);
  if (new_size != ary->ka_size) {
    ary->ka_values = lsrealloc(ary->ka_values, new_size * sizeof(void *));
    for (unsigned int i = ary->ka_size; i < new_size; i++)
      ary->ka_values[i] = NULL;
  }
  ary->ka_size = new_size;
}

lsarray_t *lsarray() {
  lsarray_t *array = lsmalloc(sizeof(lsarray_t));
  array->ka_values = NULL;
  array->ka_size = 0;
  return array;
}

void lsarray_free(lsarray_t *ary) {
  if (ary == NULL)
    return;
  if (ary->ka_values != NULL)
    lsfree(ary->ka_values);
  lsfree(ary);
}

void lsarray_set(lsarray_t *ary, unsigned int i, void *val) {
  assert(ary != NULL);
  if (i >= ary->ka_size) {
    unsigned int new_size = i + 1;
    lsarray_resize(ary, new_size);
    ary->ka_size = new_size;
  }
  ary->ka_values[i] = val;
}

void *lsarray_get(const lsarray_t *ary, unsigned int i) {
  if (ary == NULL)
    return NULL;
  if (i >= ary->ka_size)
    return NULL;
  return ary->ka_values[i];
}

unsigned int lsarray_get_size(const lsarray_t *ary) {
  return ary == NULL ? 0 : ary->ka_size;
}

void lsarray_set_size(lsarray_t *ary, unsigned int new_size) {
  assert(ary != NULL);
  lsarray_resize(ary, new_size);
  ary->ka_size = new_size;
}

void lsarray_push(lsarray_t *ary, void *val) {
  assert(ary != NULL);
  lsarray_set(ary, ary->ka_size, val);
}

void *lsarray_pop(lsarray_t *ary) {
  assert(ary != NULL);
  if (ary->ka_size == 0)
    return NULL;
  void *value = lsarray_get(ary, ary->ka_size - 1);
  lsarray_set_size(ary, ary->ka_size - 1);
  return value;
}

void lsarray_unshift(lsarray_t *ary, void *val) {
  assert(ary != NULL);
  if (ary->ka_size == 0) {
    lsarray_push(ary, val);
    return;
  }
  lsarray_set_size(ary, ary->ka_size + 1);
  for (unsigned int i = ary->ka_size - 1; i > 0; i--)
    ary->ka_values[i] = ary->ka_values[i - 1];
  ary->ka_values[0] = val;
}

void *lsarray_shift(lsarray_t *ary) {
  assert(ary != NULL);
  if (ary->ka_size == 0)
    return NULL;
  void *value = lsarray_get(ary, 0);
  for (unsigned int i = 1; i < ary->ka_size; i++)
    ary->ka_values[i - 1] = ary->ka_values[i];
  lsarray_set_size(ary, ary->ka_size - 1);
  return value;
}

lsarray_t *lsarray_clone(const lsarray_t *ary) {
  if (ary == NULL)
    return NULL;
  lsarray_t *clone = lsmalloc(sizeof(lsarray_t));
  clone->ka_values = lsmalloc(ary->ka_size * sizeof(void *));
  clone->ka_size = ary->ka_size;
  for (unsigned int i = 0; i < ary->ka_size; i++)
    clone->ka_values[i] = ary->ka_values[i];
  return clone;
}

lsarray_t *lsarray_concat(lsarray_t *ary1, const lsarray_t *ary2) {
  if (ary2 == NULL)
    return ary1;
  if (ary1 == NULL)
    return lsarray_clone(ary2);
  ary1->ka_values = lsrealloc(ary1->ka_values,
                              (ary1->ka_size + ary2->ka_size) * sizeof(void *));
  ary1->ka_size += ary2->ka_size;
  for (unsigned int i = 0; i < ary2->ka_size; i++)
    ary1->ka_values[ary1->ka_size + i] = ary2->ka_values[i];
  ary1->ka_size += ary2->ka_size;
  return ary1;
}
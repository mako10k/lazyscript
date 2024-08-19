#include "array.h"
#include "malloc.h"
#include <assert.h>
#include <stdio.h>

struct lsarray {
  void **ka_values;
  unsigned int ka_size;
  size_t ka_capacity;
  size_t ka_incr1;
  size_t ka_incr2;
};

static void lsarray_resize(lsarray_t *array, unsigned int new_size) {
  assert(array != NULL);
  if (new_size > array->ka_capacity) {
    size_t new_capacity = array->ka_capacity;
    size_t incr1 = array->ka_incr1;
    size_t incr2 = array->ka_incr2;
    while (new_capacity < new_size) {
      new_capacity = incr1 + incr2;
      incr1 = incr2;
      incr2 = new_capacity;
    }
    array->ka_values =
        lsrealloc(array->ka_values, new_capacity * sizeof(void *));
    for (unsigned int i = array->ka_size; i < new_capacity; i++)
      array->ka_values[i] = NULL;
    array->ka_capacity = new_capacity;
    array->ka_incr1 = incr1;
    array->ka_incr2 = incr2;
  } else {
    for (unsigned int i = new_size; i < array->ka_size; i++)
      array->ka_values[i] = NULL;
  }
  array->ka_size = new_size;
}

lsarray_t *lsarray(unsigned int size) {
  lsarray_t *array = lsmalloc(sizeof(lsarray_t));
  size_t capacity = 1;
  size_t incr1 = 1;
  size_t incr2 = 1;
  array->ka_values = NULL;
  array->ka_size = 0;
  array->ka_capacity = 0;
  array->ka_incr1 = incr1;
  array->ka_incr2 = incr2;
  lsarray_resize(array, size);
  for (unsigned int i = 0; i < size; i++)
    array->ka_values[i] = NULL;
  return array;
}

const lsarray_t *lsarray_const(const lsarray_t *array) {
  assert(array != NULL);
  return array;
}

void lsarray_free(lsarray_t *array) {
  assert(array != NULL);
  if (array->ka_values != NULL)
    lsfree(array->ka_values);
  lsfree(array);
}

void lsarray_set(lsarray_t *array, unsigned int index, void *value) {
  assert(array != NULL);
  if (index >= array->ka_size) {
    unsigned int new_size = index + 1;
    lsarray_resize(array, new_size);
    array->ka_size = new_size;
  }
  array->ka_values[index] = value;
}

void *lsarray_get(const lsarray_t *array, unsigned int index) {
  assert(array != NULL);
  if (index >= array->ka_size)
    return NULL;
  return array->ka_values[index];
}

unsigned int lsarray_get_size(const lsarray_t *array) {
  assert(array != NULL);
  return array->ka_size;
}

void lsarray_set_size(lsarray_t *array, unsigned int new_size) {
  assert(array != NULL);
  lsarray_resize(array, new_size);
  array->ka_size = new_size;
}

void lsarray_push(lsarray_t *array, void *value) {
  assert(array != NULL);
  lsarray_set(array, array->ka_size, value);
}

void *lsarray_pop(lsarray_t *array) {
  assert(array != NULL);
  if (array->ka_size == 0)
    return NULL;
  void *value = lsarray_get(array, array->ka_size - 1);
  lsarray_set_size(array, array->ka_size - 1);
  return value;
}

void lsarray_unshift(lsarray_t *array, void *value) {
  assert(array != NULL);
  if (array->ka_size == 0) {
    lsarray_push(array, value);
    return;
  }
  lsarray_set_size(array, array->ka_size + 1);
  for (unsigned int i = array->ka_size - 1; i > 0; i--)
    array->ka_values[i] = array->ka_values[i - 1];
  array->ka_values[0] = value;
}

void *lsarray_shift(lsarray_t *array) {
  assert(array != NULL);
  if (array->ka_size == 0)
    return NULL;
  void *value = lsarray_get(array, 0);
  for (unsigned int i = 1; i < array->ka_size; i++)
    array->ka_values[i - 1] = array->ka_values[i];
  lsarray_set_size(array, array->ka_size - 1);
  return value;
}

lsarray_t *lsarray_clone(const lsarray_t *array) {
  assert(array != NULL);
  lsarray_t *clone = lsarray(array->ka_size);
  for (unsigned int i = 0; i < array->ka_size; i++)
    lsarray_set(clone, i, lsarray_get(array, i));
  return clone;
}

lsarray_t *lsarray_concat(lsarray_t *array1, const lsarray_t *array2) {
  assert(array1 != NULL);
  assert(array2 != NULL);
  lsarray_resize(array1, array1->ka_size + array2->ka_size);
  for (unsigned int i = 0; i < array2->ka_size; i++)
    lsarray_set(array1, array1->ka_size + i, lsarray_get(array2, i));
  return array1;
}
#include "common/array.h"
#include "common/malloc.h"
#include "lstypes.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

const void *const *lsa_newv(lssize_t size, va_list ap) {
  const void **new_ary = lsmalloc(size * sizeof(void *));
  for (lssize_t i = 0; i < size; i++)
    new_ary[i] = va_arg(ap, void *);
  return new_ary;
}

const void *const *lsa_newa(lssize_t size, const void *const *ary) {
  const void **new_ary = lsmalloc(size * sizeof(void *));
  for (lssize_t i = 0; i < size; i++)
    new_ary[i] = ary[i];
  return new_ary;
}

const void *const *lsa_new(lssize_t size, ...) {
  va_list ap;
  va_start(ap, size);
  const void *const *new_ary = lsa_newv(size, ap);
  va_end(ap);
  return new_ary;
}

const void *const *lsa_push(lssize_t size, const void *const *ary,
                            const void *val) {
  const void **new_ary = lsmalloc((size + 1) * sizeof(void *));
  for (lssize_t i = 0; i < size; i++)
    new_ary[i] = ary[i];
  new_ary[size] = val;
  return new_ary;
}

const void *lsa_pop(lssize_t size, const void *const *ary,
                    const void *const **pary) {
  assert(size > 0);
  if (size == 1) {
    *pary = NULL;
    return ary[0];
  }
  *pary = ary;
  return ary[size - 1];
}

const void *const *lsa_unshift(lssize_t size, const void *const *ary,
                               const void *val) {
  const void **new_ary = lsmalloc((size + 1) * sizeof(void *));
  new_ary[0] = val;
  for (lssize_t i = 0; i < size; i++)
    new_ary[i + 1] = ary[i];
  return new_ary;
}

const void *lsa_shift(lssize_t size, const void *const *ary,
                      const void *const **pary) {
  assert(size > 0);
  if (size == 1) {
    *pary = NULL;
    return ary[0];
  }
  *pary = ary + 1;
  return ary[0];
}

const void *const *lsa_concatv(lssize_t size1, const void *const *ary1,
                               lssize_t size2, va_list ap) {
  const void **new_ary = lsmalloc((size1 + size2) * sizeof(void *));
  for (lssize_t i = 0; i < size1; i++)
    new_ary[i] = ary1[i];
  for (lssize_t i = 0; i < size2; i++)
    new_ary[size1 + i] = va_arg(ap, void *);
  return new_ary;
}

const void *const *lsa_concat(lssize_t size1, const void *const *ary1,
                              lssize_t size2, ...) {
  va_list ap;
  va_start(ap, size2);
  const void *const *new_ary = lsa_concatv(size1, ary1, size2, ap);
  va_end(ap);
  return new_ary;
}

const void *const *lsa_concata(lssize_t size1, const void *const *ary1,
                               lssize_t size2, const void *const *ary2) {
  const void **new_ary = lsmalloc((size1 + size2) * sizeof(void *));
  for (lssize_t i = 0; i < size1; i++)
    new_ary[i] = ary1[i];
  for (lssize_t i = 0; i < size2; i++)
    new_ary[size1 + i] = ary2[i];
  return new_ary;
}

const void *const *lsa_splicev(lssize_t size, const void *const *ary,
                               lssize_t start, lssize_t end, lssize_t insize,
                               va_list ap) {
  assert(start >= 0 && start <= size);
  assert(end >= 0 && end <= size);
  assert(start <= end);
  if (start == end && insize == 0)
    return ary;
  const void **new_ary =
      lsmalloc((size - end + start + insize) * sizeof(void *));
  for (lssize_t i = 0; i < start; i++)
    new_ary[i] = ary[i];
  for (lssize_t i = 0; i < insize; i++)
    new_ary[start + i] = va_arg(ap, void *);
  for (lssize_t i = end; i < size; i++)
    new_ary[start + insize + i - end] = ary[i];
  return new_ary;
}

const void *const *lsa_splicea(lssize_t size, const void *const *ary,
                               lssize_t start, lssize_t end, lssize_t insize,
                               const void *const *ins) {
  assert(start >= 0 && start <= size);
  assert(end >= 0 && end <= size);
  assert(start <= end);
  if (start == end && insize == 0)
    return ary;
  const void **new_ary =
      lsmalloc((size - end + start + insize) * sizeof(void *));
  for (lssize_t i = 0; i < start; i++)
    new_ary[i] = ary[i];
  for (lssize_t i = 0; i < insize; i++)
    new_ary[start + i] = ins[i];
  for (lssize_t i = end; i < size; i++)
    new_ary[start + insize + i - end] = ary[i];
  return new_ary;
}

const void *const *lsa_splice(lssize_t size, const void *const *ary,
                              lssize_t start, lssize_t end, lssize_t insize,
                              ...) {
  va_list ap;
  va_start(ap, insize);
  const void *const *new_ary = lsa_splicev(size, ary, start, end, insize, ap);
  va_end(ap);
  return new_ary;
}

struct lsarray {
  lssize_t la_size;
  const void *const *la_values;
};

const lsarray_t *lsarray_newv(lssize_t size, va_list ap) {
  assert(size > 0);
  const void **values = lsmalloc(size * sizeof(void *));
  for (lssize_t i = 0; i < size; i++)
    values[i] = va_arg(ap, void *);
  lsarray_t *ary = lsmalloc(sizeof(lsarray_t));
  ary->la_size = size;
  ary->la_values = values;
  return ary;
}

const lsarray_t *lsarray_newa(lssize_t size, const void *const *values) {
  assert(size == 0 || values != NULL);
  if (size == 0)
    return NULL;
  lsarray_t *ary = lsmalloc(sizeof(lsarray_t));
  ary->la_size = size;
  ary->la_values = lsa_new(size, values);
  return ary;
}

const lsarray_t *lsarray_new(lssize_t size, ...) {
  va_list ap;
  va_start(ap, size);
  const lsarray_t *ary = lsarray_newv(size, ap);
  va_end(ap);
  return ary;
}

const void *const *lsarray_get(const lsarray_t *ary) {
  if (ary == NULL)
    return NULL;
  return ary->la_values;
}

lssize_t lsarray_get_size(const lsarray_t *ary) {
  return ary == NULL ? 0 : ary->la_size;
}

const lsarray_t *lsarray_push(const lsarray_t *ary, const void *val) {
  if (ary == NULL || ary->la_size == 0)
    return lsarray_new(1, &val);
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = lsa_push(ary->la_size, ary->la_values, val);
  new_ary->la_size = lsarray_get_size(ary) + 1;
  return new_ary;
}

const void *lsarray_pop(const lsarray_t *ary, const lsarray_t **pnew_ary) {
  assert(ary != NULL && ary->la_size > 0);
  if (pnew_ary == NULL)
    return ary->la_values[ary->la_size - 1];
  if (ary->la_size == 1) {
    *pnew_ary = NULL;
    return ary->la_values[0];
  }
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  const void *val = lsa_pop(ary->la_size, ary->la_values, &new_ary->la_values);
  new_ary->la_size = ary->la_size - 1;
  *pnew_ary = new_ary;
  return val;
}

const lsarray_t *lsarray_unshift(const lsarray_t *ary, const void *val) {
  if (ary == NULL || ary->la_size == 0)
    return lsarray_new(1, &val);
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = lsa_push(ary->la_size, ary->la_values, val);
  new_ary->la_size = ary->la_size + 1;
  return new_ary;
}

const void *lsarray_shift(const lsarray_t *ary, const lsarray_t **pnew_ary) {
  assert(ary != NULL && ary->la_size > 0);
  if (pnew_ary == NULL)
    return ary->la_values[0];
  if (ary->la_size == 1) {
    *pnew_ary = NULL;
    return ary->la_values[0];
  }
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  const void *val =
      lsa_shift(ary->la_size, ary->la_values, &new_ary->la_values);
  new_ary->la_size = ary->la_size - 1;
  *pnew_ary = new_ary;
  return val;
}

const lsarray_t *lsarray_concat(const lsarray_t *ary1, const lsarray_t *ary2) {
  if (ary1 == NULL)
    return ary2;
  if (ary2 == NULL)
    return ary1;
  lsarray_t *ary = lsmalloc(sizeof(lsarray_t));
  ary->la_values = lsa_concata(ary1->la_size, ary1->la_values, ary2->la_size,
                               ary2->la_values);
  ary->la_size = ary1->la_size + ary2->la_size;
  return ary;
}

const lsarray_t *lsarray_splicev(const lsarray_t *ary, lssize_t start,
                                 lssize_t end, lssize_t insize, va_list ap) {
  assert(start >= 0 && start <= lsarray_get_size(ary));
  assert(end >= 0 && end <= lsarray_get_size(ary));
  assert(start <= end);
  if (start == end && insize == 0)
    return ary;
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values =
      lsa_splicev(ary->la_size, ary->la_values, start, end, insize, ap);
  new_ary->la_size = ary->la_size - end + start + insize;
  return new_ary;
}

const lsarray_t *lsarray_splicea(const lsarray_t *ary, lssize_t start,
                                 lssize_t end, const lsarray_t *ins) {
  assert(start >= 0 && start <= lsarray_get_size(ary));
  assert(end >= 0 && end <= lsarray_get_size(ary));
  assert(start <= end);
  if (start == end && lsarray_get_size(ins) == 0)
    return ary;
  lsarray_t *new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = lsa_splicea(ary->la_size, ary->la_values, start, end,
                                   lsarray_get_size(ins), lsarray_get(ins));
  new_ary->la_size = ary->la_size - end + start + lsarray_get_size(ins);
  return new_ary;
}

const lsarray_t *lsarray_splice(const lsarray_t *ary, lssize_t start,
                                lssize_t end, lssize_t insize, ...) {
  va_list ap;
  va_start(ap, insize);
  const lsarray_t *new_ary = lsarray_splicev(ary, start, end, insize, ap);
  va_end(ap);
  return new_ary;
}
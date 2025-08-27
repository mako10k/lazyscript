#include "common/array.h"
#include "common/malloc.h"
#include <assert.h>
#include <stdio.h>

const void* const* lsa_newv(lssize_t size, va_list ap) {
  const void** new_ary = lsmalloc(size * sizeof(void*));
  for (lssize_t i = 0; i < size; i++)
    new_ary[i] = va_arg(ap, void*);
  return new_ary;
}

// removed: lsa_newa (unused)

const void* const* lsa_new(lssize_t size, ...) {
  va_list ap;
  va_start(ap, size);
  const void* const* new_ary = lsa_newv(size, ap);
  va_end(ap);
  return new_ary;
}

const void* const* lsa_push(lssize_t size, const void* const* ary, const void* val) {
  const void** new_ary = lsmalloc((size + 1) * sizeof(void*));
  for (lssize_t i = 0; i < size; i++)
    new_ary[i] = ary[i];
  new_ary[size] = val;
  return new_ary;
}

// removed: lsa_pop (unused)

// removed: lsa_unshift (unused)

const void* const* lsa_shift(lssize_t size, const void* const* ary, const void** pval) {
  assert(size > 0);
  const void* const* new_ary = size == 1 ? NULL : ary + 1;
  if (pval != NULL)
    *pval = ary[0];
  return new_ary;
}

// removed: lsa_concatv (unused)

// removed: lsa_concat (unused)

const void* const* lsa_concata(lssize_t size1, const void* const* ary1, lssize_t size2,
                               const void* const* ary2) {
  if (size1 == 0)
    return ary2;
  if (size2 == 0)
    return ary1;
  const void** new_ary = lsmalloc((size1 + size2) * sizeof(void*));
  for (lssize_t i = 0; i < size1; i++)
    new_ary[i] = ary1[i];
  for (lssize_t i = 0; i < size2; i++)
    new_ary[size1 + i] = ary2[i];
  return new_ary;
}

// removed: lsa_slice (unused)

// removed: lsa_splicev (unused)

// removed: lsa_splicea (unused)

// removed: lsa_splice (unused)

struct lsarray {
  lssize_t           la_size;
  const void* const* la_values;
};

const lsarray_t* lsarray_newv(lssize_t size, va_list ap) {
  assert(size > 0);
  const void** values = lsmalloc(size * sizeof(void*));
  for (lssize_t i = 0; i < size; i++)
    values[i] = va_arg(ap, void*);
  lsarray_t* ary = lsmalloc(sizeof(lsarray_t));
  ary->la_size   = size;
  ary->la_values = values;
  return ary;
}

// removed: lsarray_newa (unused)

const lsarray_t* lsarray_new(lssize_t size, ...) {
  va_list ap;
  va_start(ap, size);
  const lsarray_t* ary = lsarray_newv(size, ap);
  va_end(ap);
  return ary;
}

const void* const* lsarray_get(const lsarray_t* ary) {
  if (ary == NULL)
    return NULL;
  return ary->la_values;
}

lssize_t         lsarray_get_size(const lsarray_t* ary) { return ary == NULL ? 0 : ary->la_size; }

const lsarray_t* lsarray_push(const lsarray_t* ary, const void* val) {
  if (ary == NULL || ary->la_size == 0) {
    // BUGFIX: pass val itself, not &val. Using &val stores the address of the local
    // parameter variable which becomes dangling after return, leading to crashes.
    return lsarray_new(1, val);
  }
  lsarray_t* new_ary = lsmalloc(sizeof(lsarray_t));
  new_ary->la_values = lsa_push(ary->la_size, ary->la_values, val);
  new_ary->la_size   = lsarray_get_size(ary) + 1;
  return new_ary;
}

// removed: lsarray_pop (unused)

// removed: lsarray_unshift (unused)

// removed: lsarray_shift (unused)

// removed: lsarray_concat (unused)

// removed: lsarray_splicev (unused)

// removed: lsarray_splicea (unused)

// removed: lsarray_splice (unused)
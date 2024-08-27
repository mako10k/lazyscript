#include "belist.h"
#include "malloc.h"
#include <assert.h>

struct lsbelist {
  lsbind_entry_t *lbel_entry;
  const lsbelist_t *lbel_next;
};

struct lsbelistm {
  lsbind_entry_t *lbelm_entry;
  lsbelistm_t *lbelm_next;
};

lsbelistm_t *lsbelistm_new(void) { return NULL; }

void lsbelistm_push(lsbelistm_t **pbelistm, lsbind_entry_t *expr) {
  assert(pbelistm != NULL);
  assert(expr != NULL);
  while (*pbelistm != NULL)
    pbelistm = &(*pbelistm)->lbelm_next;
  *pbelistm = lsmalloc(sizeof(lsbelistm_t));
  (*pbelistm)->lbelm_entry = expr;
  (*pbelistm)->lbelm_next = NULL;
}

lsbind_entry_t *lsbelistm_pop(lsbelistm_t **pbelistm) {
  assert(pbelistm != NULL);
  if (*pbelistm == NULL)
    return NULL;
  while ((*pbelistm)->lbelm_next != NULL)
    pbelistm = &(*pbelistm)->lbelm_next;
  lsbind_entry_t *expr = (*pbelistm)->lbelm_entry;
  *pbelistm = NULL;
  return expr;
}

void lsbelistm_unshift(lsbelistm_t **pbelistm, lsbind_entry_t *expr) {
  assert(pbelistm != NULL);
  assert(expr != NULL);
  lsbelistm_t *belistm = lsmalloc(sizeof(lsbelistm_t));
  belistm->lbelm_entry = expr;
  belistm->lbelm_next = *pbelistm;
  *pbelistm = belistm;
}

lsbind_entry_t *lsbelistm_shift(lsbelistm_t **pbelistm) {
  assert(pbelistm != NULL);
  if (*pbelistm == NULL)
    return NULL;
  lsbind_entry_t *expr = (*pbelistm)->lbelm_entry;
  lsbelistm_t *next = (*pbelistm)->lbelm_next;
  *pbelistm = next;
  return expr;
}

lssize_t lsbelistm_count(const lsbelistm_t *belistm) {
  lssize_t count = 0;
  while (belistm != NULL) {
    count++;
    belistm = belistm->lbelm_next;
  }
  return count;
}

lsbind_entry_t *lsbelistm_get(const lsbelistm_t *belist, lssize_t i) {
  while (i > 0) {
    if (belist == NULL)
      return NULL;
    belist = belist->lbelm_next;
    i--;
  }
  return belist == NULL ? NULL : belist->lbelm_entry;
}

lsbelistm_t *lsbelistm_clone(const lsbelistm_t *belistm) {
  if (belistm == NULL)
    return NULL;
  lsbelistm_t *clone = lsmalloc(sizeof(lsbelistm_t));
  clone->lbelm_entry = belistm->lbelm_entry;
  clone->lbelm_next = lsbelistm_clone(belistm->lbelm_next);
  return clone;
}

void lsbelistm_concat(lsbelistm_t **pbelistm, lsbelist_t *belistm) {
  assert(pbelistm != NULL);
  while (*pbelistm != NULL)
    pbelistm = &(*pbelistm)->lbelm_next;
  *pbelistm = lsmalloc(sizeof(lsbelistm_t));
  (*pbelistm)->lbelm_entry = belistm->lbel_entry;
  (*pbelistm)->lbelm_next = NULL;
}

const lsbelist_t *lsbelist_new(void) { return NULL; }

const lsbelist_t *lsbelist_push(const lsbelist_t *belist,
                                lsbind_entry_t *expr) {
  if (belist == NULL) {
    lsbelist_t *new_belist = lsmalloc(sizeof(lsbelist_t));
    new_belist->lbel_entry = expr;
    new_belist->lbel_next = NULL;
    return new_belist;
  }
  lsbelist_t *new_belist = lsmalloc(sizeof(lsbelist_t));
  new_belist->lbel_entry = belist->lbel_entry;
  new_belist->lbel_next = lsbelist_push(belist->lbel_next, expr);
  return new_belist;
}

const lsbelist_t *lsbelist_pop(const lsbelist_t *belist,
                               lsbind_entry_t **pexpr) {
  if (belist == NULL)
    return NULL;
  if (belist->lbel_next == NULL) {
    *pexpr = belist->lbel_entry;
    return NULL;
  }
  lsbelist_t *new_belist = lsmalloc(sizeof(lsbelist_t));
  new_belist->lbel_entry = belist->lbel_entry;
  new_belist->lbel_next = lsbelist_pop(belist->lbel_next, pexpr);
  return new_belist;
}

const lsbelist_t *lsbelist_unshift(const lsbelist_t *belist,
                                   lsbind_entry_t *expr) {
  lsbelist_t *new_belist = lsmalloc(sizeof(lsbelist_t));
  new_belist->lbel_entry = expr;
  new_belist->lbel_next = belist;
  return new_belist;
}

const lsbelist_t *lsbelist_shift(const lsbelist_t *belist,
                                 lsbind_entry_t **pexpr) {
  if (belist == NULL)
    return NULL;
  *pexpr = belist->lbel_entry;
  return belist->lbel_next;
}

lssize_t lsbelist_count(const lsbelist_t *belist) {
  lssize_t count = 0;
  while (belist != NULL) {
    count++;
    belist = belist->lbel_next;
  }
  return count;
}

lsbind_entry_t *lsbelist_get(const lsbelist_t *belist, lssize_t i) {
  while (i > 0) {
    if (belist == NULL)
      return NULL;
    belist = belist->lbel_next;
    i--;
  }
  return belist == NULL ? NULL : belist->lbel_entry;
}

const lsbelist_t *lsbelist_concat(const lsbelist_t *belist1,
                                  const lsbelist_t *belist2) {
  if (belist1 == NULL)
    return belist2;
  lsbelist_t *new_belist = lsmalloc(sizeof(lsbelist_t));
  new_belist->lbel_entry = belist1->lbel_entry;
  new_belist->lbel_next = lsbelist_concat(belist1->lbel_next, belist2);
  return new_belist;
}

const lsbelist_t *lsbelist_get_next(const lsbelist_t *belist) {
  return belist == NULL ? NULL : belist->lbel_next;
}
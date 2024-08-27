#include "elist.h"
#include "malloc.h"
#include <assert.h>

struct lselist {
  lsexpr_t *expr;
  const lselist_t *next;
};

struct lselistm {
  lsexpr_t *expr;
  lselistm_t *next;
};

lselistm_t *lselistm_new(void) { return NULL; }

void lselistm_push(lselistm_t **pelistm, lsexpr_t *expr) {
  assert(pelistm != NULL);
  assert(expr != NULL);
  while (*pelistm != NULL)
    pelistm = &(*pelistm)->next;
  *pelistm = lsmalloc(sizeof(lselistm_t));
  (*pelistm)->expr = expr;
  (*pelistm)->next = NULL;
}

lsexpr_t *lselistm_pop(lselistm_t **pelistm) {
  assert(pelistm != NULL);
  if (*pelistm == NULL)
    return NULL;
  while ((*pelistm)->next != NULL)
    pelistm = &(*pelistm)->next;
  lsexpr_t *expr = (*pelistm)->expr;
  *pelistm = NULL;
  return expr;
}

void lselistm_unshift(lselistm_t **pelistm, lsexpr_t *expr) {
  assert(pelistm != NULL);
  assert(expr != NULL);
  lselistm_t *elistm = lsmalloc(sizeof(lselistm_t));
  elistm->expr = expr;
  elistm->next = *pelistm;
  *pelistm = elistm;
}

lsexpr_t *lselistm_shift(lselistm_t **pelistm) {
  assert(pelistm != NULL);
  if (*pelistm == NULL)
    return NULL;
  lsexpr_t *expr = (*pelistm)->expr;
  lselistm_t *next = (*pelistm)->next;
  *pelistm = next;
  return expr;
}

lssize_t lselistm_count(const lselistm_t *elistm) {
  lssize_t count = 0;
  while (elistm != NULL) {
    count++;
    elistm = elistm->next;
  }
  return count;
}

lsexpr_t *lselistm_get(const lselistm_t *elist, lssize_t i) {
  while (i > 0) {
    if (elist == NULL)
      return NULL;
    elist = elist->next;
    i--;
  }
  return elist == NULL ? NULL : elist->expr;
}

lselistm_t *lselistm_clone(const lselistm_t *elistm) {
  if (elistm == NULL)
    return NULL;
  lselistm_t *clone = lsmalloc(sizeof(lselistm_t));
  clone->expr = elistm->expr;
  clone->next = lselistm_clone(elistm->next);
  return clone;
}

void lselistm_concat(lselistm_t **pelistm, lselist_t *elistm) {
  assert(pelistm != NULL);
  while (*pelistm != NULL)
    pelistm = &(*pelistm)->next;
  *pelistm = lsmalloc(sizeof(lselistm_t));
  (*pelistm)->expr = elistm->expr;
  (*pelistm)->next = NULL;
}

const lselist_t *lselist_new(void) { return NULL; }

const lselist_t *lselist_push(const lselist_t *elist, lsexpr_t *expr) {
  if (elist == NULL) {
    lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
    new_elist->expr = expr;
    new_elist->next = NULL;
    return new_elist;
  }
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->expr = elist->expr;
  new_elist->next = lselist_push(elist->next, expr);
  return new_elist;
}

const lselist_t *lselist_pop(const lselist_t *elist, lsexpr_t **pexpr) {
  if (elist == NULL)
    return NULL;
  if (elist->next == NULL) {
    *pexpr = elist->expr;
    return NULL;
  }
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->expr = elist->expr;
  new_elist->next = lselist_pop(elist->next, pexpr);
  return new_elist;
}

const lselist_t *lselist_unshift(const lselist_t *elist, lsexpr_t *expr) {
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->expr = expr;
  new_elist->next = elist;
  return new_elist;
}

const lselist_t *lselist_shift(const lselist_t *elist, lsexpr_t **pexpr) {
  if (elist == NULL)
    return NULL;
  *pexpr = elist->expr;
  return elist->next;
}

lssize_t lselist_count(const lselist_t *elist) {
  lssize_t count = 0;
  while (elist != NULL) {
    count++;
    elist = elist->next;
  }
  return count;
}

lsexpr_t *lselist_get(const lselist_t *elist, lssize_t i) {
  while (i > 0) {
    if (elist == NULL)
      return NULL;
    elist = elist->next;
    i--;
  }
  return elist == NULL ? NULL : elist->expr;
}

const lselist_t *lselist_concat(const lselist_t *elist1,
                                const lselist_t *elist2) {
  if (elist1 == NULL)
    return elist2;
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->expr = elist1->expr;
  new_elist->next = lselist_concat(elist1->next, elist2);
  return new_elist;
}

const lselist_t *lselist_get_next(const lselist_t *elist) {
  return elist == NULL ? NULL : elist->next;
}
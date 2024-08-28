#include "expr/elist.h"
#include "common/malloc.h"
#include <assert.h>

struct lselist {
  lsexpr_t *lel_expr;
  const lselist_t *lel_next;
};

struct lselistm {
  lsexpr_t *lelm_expr;
  lselistm_t *lelm_next;
};

lselistm_t *lselistm_new(void) { return NULL; }

void lselistm_push(lselistm_t **pelistm, lsexpr_t *expr) {
  assert(pelistm != NULL);
  assert(expr != NULL);
  while (*pelistm != NULL)
    pelistm = &(*pelistm)->lelm_next;
  *pelistm = lsmalloc(sizeof(lselistm_t));
  (*pelistm)->lelm_expr = expr;
  (*pelistm)->lelm_next = NULL;
}

lsexpr_t *lselistm_pop(lselistm_t **pelistm) {
  assert(pelistm != NULL);
  if (*pelistm == NULL)
    return NULL;
  while ((*pelistm)->lelm_next != NULL)
    pelistm = &(*pelistm)->lelm_next;
  lsexpr_t *expr = (*pelistm)->lelm_expr;
  *pelistm = NULL;
  return expr;
}

void lselistm_unshift(lselistm_t **pelistm, lsexpr_t *expr) {
  assert(pelistm != NULL);
  assert(expr != NULL);
  lselistm_t *elistm = lsmalloc(sizeof(lselistm_t));
  elistm->lelm_expr = expr;
  elistm->lelm_next = *pelistm;
  *pelistm = elistm;
}

lsexpr_t *lselistm_shift(lselistm_t **pelistm) {
  assert(pelistm != NULL);
  if (*pelistm == NULL)
    return NULL;
  lsexpr_t *expr = (*pelistm)->lelm_expr;
  lselistm_t *next = (*pelistm)->lelm_next;
  *pelistm = next;
  return expr;
}

lssize_t lselistm_count(const lselistm_t *elistm) {
  lssize_t count = 0;
  while (elistm != NULL) {
    count++;
    elistm = elistm->lelm_next;
  }
  return count;
}

lsexpr_t *lselistm_get(const lselistm_t *elist, lssize_t i) {
  while (i > 0) {
    if (elist == NULL)
      return NULL;
    elist = elist->lelm_next;
    i--;
  }
  return elist == NULL ? NULL : elist->lelm_expr;
}

lselistm_t *lselistm_clone(const lselistm_t *elistm) {
  if (elistm == NULL)
    return NULL;
  lselistm_t *clone = lsmalloc(sizeof(lselistm_t));
  clone->lelm_expr = elistm->lelm_expr;
  clone->lelm_next = lselistm_clone(elistm->lelm_next);
  return clone;
}

void lselistm_concat(lselistm_t **pelistm, lselist_t *elistm) {
  assert(pelistm != NULL);
  while (*pelistm != NULL)
    pelistm = &(*pelistm)->lelm_next;
  *pelistm = lsmalloc(sizeof(lselistm_t));
  (*pelistm)->lelm_expr = elistm->lel_expr;
  (*pelistm)->lelm_next = NULL;
}

const lselist_t *lselist_new(void) { return NULL; }

const lselist_t *lselist_push(const lselist_t *elist, lsexpr_t *expr) {
  if (elist == NULL) {
    lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
    new_elist->lel_expr = expr;
    new_elist->lel_next = NULL;
    return new_elist;
  }
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->lel_expr = elist->lel_expr;
  new_elist->lel_next = lselist_push(elist->lel_next, expr);
  return new_elist;
}

const lselist_t *lselist_pop(const lselist_t *elist, lsexpr_t **pexpr) {
  if (elist == NULL)
    return NULL;
  if (elist->lel_next == NULL) {
    *pexpr = elist->lel_expr;
    return NULL;
  }
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->lel_expr = elist->lel_expr;
  new_elist->lel_next = lselist_pop(elist->lel_next, pexpr);
  return new_elist;
}

const lselist_t *lselist_unshift(const lselist_t *elist, lsexpr_t *expr) {
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->lel_expr = expr;
  new_elist->lel_next = elist;
  return new_elist;
}

const lselist_t *lselist_shift(const lselist_t *elist, lsexpr_t **pexpr) {
  if (elist == NULL)
    return NULL;
  *pexpr = elist->lel_expr;
  return elist->lel_next;
}

lssize_t lselist_count(const lselist_t *elist) {
  lssize_t count = 0;
  while (elist != NULL) {
    count++;
    elist = elist->lel_next;
  }
  return count;
}

lsexpr_t *lselist_get(const lselist_t *elist, lssize_t i) {
  while (i > 0) {
    if (elist == NULL)
      return NULL;
    elist = elist->lel_next;
    i--;
  }
  return elist == NULL ? NULL : elist->lel_expr;
}

const lselist_t *lselist_concat(const lselist_t *elist1,
                                const lselist_t *elist2) {
  if (elist1 == NULL)
    return elist2;
  lselist_t *new_elist = lsmalloc(sizeof(lselist_t));
  new_elist->lel_expr = elist1->lel_expr;
  new_elist->lel_next = lselist_concat(elist1->lel_next, elist2);
  return new_elist;
}

const lselist_t *lselist_get_next(const lselist_t *elist) {
  return elist == NULL ? NULL : elist->lel_next;
}
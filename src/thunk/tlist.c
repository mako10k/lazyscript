#include "thunk/tlist.h"
#include "common/malloc.h"
#include <assert.h>

struct lstlist {
  lsthunk_t *ltl_thunk;
  const lstlist_t *ltl_next;
};

struct lstlistm {
  lsthunk_t *thunk;
  lstlistm_t *next;
};

lstlistm_t *lstlistm_new(void) { return NULL; }

void lstlistm_push(lstlistm_t **ptlistm, lsthunk_t *thunk) {
  assert(ptlistm != NULL);
  assert(thunk != NULL);
  while (*ptlistm != NULL)
    ptlistm = &(*ptlistm)->next;
  *ptlistm = lsmalloc(sizeof(lstlistm_t));
  (*ptlistm)->thunk = thunk;
  (*ptlistm)->next = NULL;
}

lsthunk_t *lstlistm_pop(lstlistm_t **ptlistm) {
  assert(ptlistm != NULL);
  if (*ptlistm == NULL)
    return NULL;
  while ((*ptlistm)->next != NULL)
    ptlistm = &(*ptlistm)->next;
  lsthunk_t *thunk = (*ptlistm)->thunk;
  *ptlistm = NULL;
  return thunk;
}

void lstlistm_unshift(lstlistm_t **ptlistm, lsthunk_t *thunk) {
  assert(ptlistm != NULL);
  assert(thunk != NULL);
  lstlistm_t *tlistm = lsmalloc(sizeof(lstlistm_t));
  tlistm->thunk = thunk;
  tlistm->next = *ptlistm;
  *ptlistm = tlistm;
}

lsthunk_t *lstlistm_shift(lstlistm_t **ptlistm) {
  assert(ptlistm != NULL);
  if (*ptlistm == NULL)
    return NULL;
  lsthunk_t *thunk = (*ptlistm)->thunk;
  lstlistm_t *next = (*ptlistm)->next;
  *ptlistm = next;
  return thunk;
}

lssize_t lstlistm_count(const lstlistm_t *tlistm) {
  lssize_t count = 0;
  while (tlistm != NULL) {
    count++;
    tlistm = tlistm->next;
  }
  return count;
}

lsthunk_t *lstlistm_get(const lstlistm_t *tlist, lssize_t i) {
  while (i > 0) {
    if (tlist == NULL)
      return NULL;
    tlist = tlist->next;
    i--;
  }
  return tlist == NULL ? NULL : tlist->thunk;
}

lstlistm_t *lstlistm_clone(const lstlistm_t *tlistm) {
  if (tlistm == NULL)
    return NULL;
  lstlistm_t *clone = lsmalloc(sizeof(lstlistm_t));
  clone->thunk = tlistm->thunk;
  clone->next = lstlistm_clone(tlistm->next);
  return clone;
}

void lstlistm_concat(lstlistm_t **ptlistm, lstlist_t *tlistm) {
  assert(ptlistm != NULL);
  while (*ptlistm != NULL)
    ptlistm = &(*ptlistm)->next;
  *ptlistm = lsmalloc(sizeof(lstlistm_t));
  (*ptlistm)->thunk = tlistm->ltl_thunk;
  (*ptlistm)->next = NULL;
}

const lstlist_t *lstlist_new(void) { return NULL; }

const lstlist_t *lstlist_push(const lstlist_t *tlist, lsthunk_t *thunk) {
  if (tlist == NULL) {
    lstlist_t *new_tlist = lsmalloc(sizeof(lstlist_t));
    new_tlist->ltl_thunk = thunk;
    new_tlist->ltl_next = NULL;
    return new_tlist;
  }
  lstlist_t *new_tlist = lsmalloc(sizeof(lstlist_t));
  new_tlist->ltl_thunk = tlist->ltl_thunk;
  new_tlist->ltl_next = lstlist_push(tlist->ltl_next, thunk);
  return new_tlist;
}

const lstlist_t *lstlist_pop(const lstlist_t *tlist, lsthunk_t **pthunk) {
  if (tlist == NULL)
    return NULL;
  if (tlist->ltl_next == NULL) {
    *pthunk = tlist->ltl_thunk;
    return NULL;
  }
  lstlist_t *new_tlist = lsmalloc(sizeof(lstlist_t));
  new_tlist->ltl_thunk = tlist->ltl_thunk;
  new_tlist->ltl_next = lstlist_pop(tlist->ltl_next, pthunk);
  return new_tlist;
}

const lstlist_t *lstlist_unshift(const lstlist_t *tlist, lsthunk_t *thunk) {
  lstlist_t *new_tlist = lsmalloc(sizeof(lstlist_t));
  new_tlist->ltl_thunk = thunk;
  new_tlist->ltl_next = tlist;
  return new_tlist;
}

const lstlist_t *lstlist_shift(const lstlist_t *tlist, lsthunk_t **pthunk) {
  if (tlist == NULL)
    return NULL;
  *pthunk = tlist->ltl_thunk;
  return tlist->ltl_next;
}

lssize_t lstlist_count(const lstlist_t *tlist) {
  lssize_t count = 0;
  while (tlist != NULL) {
    count++;
    tlist = tlist->ltl_next;
  }
  return count;
}

lsthunk_t *lstlist_get(const lstlist_t *tlist, lssize_t i) {
  while (i > 0) {
    if (tlist == NULL)
      return NULL;
    tlist = tlist->ltl_next;
    i--;
  }
  return tlist == NULL ? NULL : tlist->ltl_thunk;
}

const lstlist_t *lstlist_concat(const lstlist_t *tlist1,
                                const lstlist_t *tlist2) {
  if (tlist1 == NULL)
    return tlist2;
  lstlist_t *new_tlist = lsmalloc(sizeof(lstlist_t));
  new_tlist->ltl_thunk = tlist1->ltl_thunk;
  new_tlist->ltl_next = lstlist_concat(tlist1->ltl_next, tlist2);
  return new_tlist;
}

const lstlist_t *lstlist_get_next(const lstlist_t *tlist) {
  return tlist == NULL ? NULL : tlist->ltl_next;
}
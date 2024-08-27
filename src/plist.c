#include "plist.h"
#include "malloc.h"
#include <assert.h>

struct lsplist {
  lspat_t *pat;
  const lsplist_t *next;
};

struct lsplistm {
  lspat_t *pat;
  lsplistm_t *next;
};

lsplistm_t *lsplistm_new(void) { return NULL; }

void lsplistm_push(lsplistm_t **pplistm, lspat_t *pat) {
  assert(pplistm != NULL);
  assert(pat != NULL);
  while (*pplistm != NULL)
    pplistm = &(*pplistm)->next;
  *pplistm = lsmalloc(sizeof(lsplistm_t));
  (*pplistm)->pat = pat;
  (*pplistm)->next = NULL;
}

lspat_t *lsplistm_pop(lsplistm_t **pplistm) {
  assert(pplistm != NULL);
  if (*pplistm == NULL)
    return NULL;
  while ((*pplistm)->next != NULL)
    pplistm = &(*pplistm)->next;
  lspat_t *pat = (*pplistm)->pat;
  *pplistm = NULL;
  return pat;
}

void lsplistm_unshift(lsplistm_t **pplistm, lspat_t *pat) {
  assert(pplistm != NULL);
  assert(pat != NULL);
  lsplistm_t *plistm = lsmalloc(sizeof(lsplistm_t));
  plistm->pat = pat;
  plistm->next = *pplistm;
  *pplistm = plistm;
}

lspat_t *lsplistm_shift(lsplistm_t **pplistm) {
  assert(pplistm != NULL);
  if (*pplistm == NULL)
    return NULL;
  lspat_t *pat = (*pplistm)->pat;
  lsplistm_t *next = (*pplistm)->next;
  *pplistm = next;
  return pat;
}

lssize_t lsplistm_count(const lsplistm_t *plistm) {
  lssize_t count = 0;
  while (plistm != NULL) {
    count++;
    plistm = plistm->next;
  }
  return count;
}

lspat_t *lsplistm_get(const lsplistm_t *plist, lssize_t i) {
  while (i > 0) {
    if (plist == NULL)
      return NULL;
    plist = plist->next;
    i--;
  }
  return plist == NULL ? NULL : plist->pat;
}

lsplistm_t *lsplistm_clone(const lsplistm_t *plistm) {
  if (plistm == NULL)
    return NULL;
  lsplistm_t *clone = lsmalloc(sizeof(lsplistm_t));
  clone->pat = plistm->pat;
  clone->next = lsplistm_clone(plistm->next);
  return clone;
}

void lsplistm_concat(lsplistm_t **pplistm, lsplist_t *plistm) {
  assert(pplistm != NULL);
  while (*pplistm != NULL)
    pplistm = &(*pplistm)->next;
  *pplistm = lsmalloc(sizeof(lsplistm_t));
  (*pplistm)->pat = plistm->pat;
  (*pplistm)->next = NULL;
}

const lsplist_t *lsplist_new(void) { return NULL; }

const lsplist_t *lsplist_push(const lsplist_t *plist, lspat_t *pat) {
  if (plist == NULL) {
    lsplist_t *new_plist = lsmalloc(sizeof(lsplist_t));
    new_plist->pat = pat;
    new_plist->next = NULL;
    return new_plist;
  }
  lsplist_t *new_plist = lsmalloc(sizeof(lsplist_t));
  new_plist->pat = plist->pat;
  new_plist->next = lsplist_push(plist->next, pat);
  return new_plist;
}

const lsplist_t *lsplist_pop(const lsplist_t *plist, lspat_t **ppat) {
  if (plist == NULL)
    return NULL;
  if (plist->next == NULL) {
    *ppat = plist->pat;
    return NULL;
  }
  lsplist_t *new_plist = lsmalloc(sizeof(lsplist_t));
  new_plist->pat = plist->pat;
  new_plist->next = lsplist_pop(plist->next, ppat);
  return new_plist;
}

const lsplist_t *lsplist_unshift(const lsplist_t *plist, lspat_t *pat) {
  lsplist_t *new_plist = lsmalloc(sizeof(lsplist_t));
  new_plist->pat = pat;
  new_plist->next = plist;
  return new_plist;
}

const lsplist_t *lsplist_shift(const lsplist_t *plist, lspat_t **ppat) {
  if (plist == NULL)
    return NULL;
  *ppat = plist->pat;
  return plist->next;
}

lssize_t lsplist_count(const lsplist_t *plist) {
  lssize_t count = 0;
  while (plist != NULL) {
    count++;
    plist = plist->next;
  }
  return count;
}

lspat_t *lsplist_get(const lsplist_t *plist, lssize_t i) {
  while (i > 0) {
    if (plist == NULL)
      return NULL;
    plist = plist->next;
    i--;
  }
  return plist == NULL ? NULL : plist->pat;
}

const lsplist_t *lsplist_concat(const lsplist_t *plist1,
                                const lsplist_t *plist2) {
  if (plist1 == NULL)
    return plist2;
  lsplist_t *new_plist = lsmalloc(sizeof(lsplist_t));
  new_plist->pat = plist1->pat;
  new_plist->next = lsplist_concat(plist1->next, plist2);
  return new_plist;
}

const lsplist_t *lsplist_get_next(const lsplist_t *plist) {
  return plist == NULL ? NULL : plist->next;
}
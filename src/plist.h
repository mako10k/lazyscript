#pragma once

typedef struct lsplist lsplist_t;
typedef struct lsplistm lsplistm_t;

#include "pat.h"

lsplistm_t *lsplistm_new(void);
void lsplistm_push(lsplistm_t **pplistm, lspat_t *pat);
lspat_t *lsplistm_pop(lsplistm_t **pplistm);
void lsplistm_unshift(lsplistm_t **pplistm, lspat_t *pat);
lspat_t *lsplistm_shift(lsplistm_t **pplistm);
lssize_t lsplistm_count(const lsplistm_t *plistm);
lspat_t *lsplistm_get(const lsplistm_t *plist, lssize_t i);
lsplistm_t *lsplistm_clone(const lsplistm_t *plistm);
void lsplistm_concat(lsplistm_t **pplistm, lsplist_t *plistm);

const lsplist_t *lsplist_new(void);
const lsplist_t *lsplist_push(const lsplist_t *plist, lspat_t *pat);
const lsplist_t *lsplist_pop(const lsplist_t *plist, lspat_t **ppat);
const lsplist_t *lsplist_unshift(const lsplist_t *plist, lspat_t *pat);
const lsplist_t *lsplist_shift(const lsplist_t *plist, lspat_t **ppat);
lssize_t lsplist_count(const lsplist_t *plist);
lspat_t *lsplist_get(const lsplist_t *plist, lssize_t i);
const lsplist_t *lsplist_concat(const lsplist_t *plist1,
                                const lsplist_t *plist2);
const lsplist_t *lsplist_get_next(const lsplist_t *plist);

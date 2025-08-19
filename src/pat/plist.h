#pragma once

typedef struct lsplist lsplist_t;

#include "pat/pat.h"

const lsplist_t* lsplist_new(void);
const lsplist_t* lsplist_push(const lsplist_t* plist, const lspat_t* pat);
const lsplist_t* lsplist_pop(const lsplist_t* plist, const lspat_t** ppat);
const lsplist_t* lsplist_unshift(const lsplist_t* plist, const lspat_t* pat);
const lsplist_t* lsplist_shift(const lsplist_t* plist, const lspat_t** ppat);
lssize_t         lsplist_count(const lsplist_t* plist);
const lspat_t*   lsplist_get(const lsplist_t* plist, lssize_t i);
const lsplist_t* lsplist_concat(const lsplist_t* plist1, const lsplist_t* plist2);
const lsplist_t* lsplist_get_next(const lsplist_t* plist);

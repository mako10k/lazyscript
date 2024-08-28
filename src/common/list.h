#pragma once

typedef struct lslist lslist_t;
typedef void *lsdata_t;

#include "lstypes.h"

const lslist_t *lslist_new(void);
const lslist_t *lslist_push(const lslist_t *tlist, lsdata_t data);
const lslist_t *lslist_pop(const lslist_t *tlist, lsdata_t *pdata);
const lslist_t *lslist_unshift(const lslist_t *tlist, lsdata_t data);
const lslist_t *lslist_shift(const lslist_t *tlist, lsdata_t *pdata);
lssize_t lslist_count(const lslist_t *tlist);
lsdata_t lslist_get(const lslist_t *tlist, lssize_t i);
const lslist_t *lslist_concat(const lslist_t *tlist1, const lslist_t *tlist2);
const lslist_t *lslist_get_next(const lslist_t *tlist);

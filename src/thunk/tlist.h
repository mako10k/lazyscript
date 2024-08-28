#pragma once

typedef struct lstlist lstlist_t;

#include "thunk/thunk.h"

const lstlist_t *lstlist_new(void);
const lstlist_t *lstlist_push(const lstlist_t *tlist, lsthunk_t *thunk);
const lstlist_t *lstlist_pop(const lstlist_t *tlist, lsthunk_t **pthunk);
const lstlist_t *lstlist_unshift(const lstlist_t *tlist, lsthunk_t *thunk);
const lstlist_t *lstlist_shift(const lstlist_t *tlist, lsthunk_t **pthunk);
lssize_t lstlist_count(const lstlist_t *tlist);
lsthunk_t *lstlist_get(const lstlist_t *tlist, lssize_t i);
const lstlist_t *lstlist_concat(const lstlist_t *tlist1,
                                const lstlist_t *tlist2);
const lstlist_t *lstlist_get_next(const lstlist_t *tlist);

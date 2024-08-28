#pragma once

typedef struct lstlist lstlist_t;
typedef struct lstlistm lstlistm_t;

#include "thunk/thunk.h"

lstlistm_t *lstlistm_new(void);
void lstlistm_push(lstlistm_t **ptlistm, lsthunk_t *thunk);
lsthunk_t *lstlistm_pop(lstlistm_t **ptlistm);
void lstlistm_unshift(lstlistm_t **ptlistm, lsthunk_t *thunk);
lsthunk_t *lstlistm_shift(lstlistm_t **ptlistm);
lssize_t lstlistm_count(const lstlistm_t *tlistm);
lsthunk_t *lstlistm_get(const lstlistm_t *tlist, lssize_t i);
lstlistm_t *lstlistm_clone(const lstlistm_t *tlistm);
void lstlistm_concat(lstlistm_t **ptlistm, lstlist_t *tlistm);

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

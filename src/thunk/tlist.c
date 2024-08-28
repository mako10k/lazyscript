#include "thunk/tlist.h"
#include "common/list.h"
#include <assert.h>

struct lstlist {
  lsthunk_t *ltl_thunk;
  const lstlist_t *ltl_next;
};

const lstlist_t *lstlist_new(void) { return (const lstlist_t *)lslist_new(); }

const lstlist_t *lstlist_push(const lstlist_t *tlist, lsthunk_t *thunk) {
  return (const lstlist_t *)lslist_push((const lslist_t *)tlist, thunk);
}

const lstlist_t *lstlist_pop(const lstlist_t *tlist, lsthunk_t **pthunk) {
  return (const lstlist_t *)lslist_pop((const lslist_t *)tlist,
                                       (lslist_data_t *)pthunk);
}

const lstlist_t *lstlist_unshift(const lstlist_t *tlist, lsthunk_t *thunk) {
  return (const lstlist_t *)lslist_unshift((const lslist_t *)tlist, thunk);
}

const lstlist_t *lstlist_shift(const lstlist_t *tlist, lsthunk_t **pthunk) {
  return (const lstlist_t *)lslist_shift((const lslist_t *)tlist,
                                         (lslist_data_t *)pthunk);
}

lssize_t lstlist_count(const lstlist_t *tlist) {
  return lslist_count((const lslist_t *)tlist);
}

lsthunk_t *lstlist_get(const lstlist_t *tlist, lssize_t i) {
  return (lsthunk_t *)lslist_get((const lslist_t *)tlist, i);
}

const lstlist_t *lstlist_concat(const lstlist_t *tlist1,
                                const lstlist_t *tlist2) {
  return (const lstlist_t *)lslist_concat((const lslist_t *)tlist1,
                                          (const lslist_t *)tlist2);
}

const lstlist_t *lstlist_get_next(const lstlist_t *tlist) {
  return (const lstlist_t *)lslist_get_next((const lslist_t *)tlist);
}
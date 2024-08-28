#include "expr/elist.h"
#include "common/list.h"
#include <assert.h>

struct lselist {
  lsexpr_t *lel_expr;
  const lselist_t *lel_next;
};

const lselist_t *lselist_new(void) { return (const lselist_t *)lslist_new(); }

const lselist_t *lselist_push(const lselist_t *elist, lsexpr_t *expr) {
  return (const lselist_t *)lslist_push((const lslist_t *)elist, expr);
}

const lselist_t *lselist_pop(const lselist_t *elist, lsexpr_t **pexpr) {
  return (const lselist_t *)lslist_pop((const lslist_t *)elist,
                                       (lslist_data_t *)pexpr);
}

const lselist_t *lselist_unshift(const lselist_t *elist, lsexpr_t *expr) {
  return (const lselist_t *)lslist_unshift((const lslist_t *)elist, expr);
}

const lselist_t *lselist_shift(const lselist_t *elist, lsexpr_t **pexpr) {
  return (const lselist_t *)lslist_shift((const lslist_t *)elist,
                                         (lslist_data_t *)pexpr);
}

lssize_t lselist_count(const lselist_t *elist) {
  return lslist_count((const lslist_t *)elist);
}

lsexpr_t *lselist_get(const lselist_t *elist, lssize_t i) {
  return (lsexpr_t *)lslist_get((const lslist_t *)elist, i);
}

const lselist_t *lselist_concat(const lselist_t *elist1,
                                const lselist_t *elist2) {
  return (const lselist_t *)lslist_concat((const lslist_t *)elist1,
                                          (const lslist_t *)elist2);
}

const lselist_t *lselist_get_next(const lselist_t *elist) {
  return (const lselist_t *)lslist_get_next((const lslist_t *)elist);
}
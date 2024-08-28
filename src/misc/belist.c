#include "misc/belist.h"
#include "common/list.h"
#include <assert.h>

struct lsbelist {
  lsbind_entry_t *lbel_entry;
  const lsbelist_t *lbel_next;
};

const lsbelist_t *lsbelist_new(void) {
  return (const lsbelist_t *)lslist_new();
}

const lsbelist_t *lsbelist_push(const lsbelist_t *belist, lsbind_entry_t *ent) {
  return (const lsbelist_t *)lslist_push((const lslist_t *)belist, ent);
}

const lsbelist_t *lsbelist_pop(const lsbelist_t *belist,
                               lsbind_entry_t **pent) {
  return (const lsbelist_t *)lslist_pop((const lslist_t *)belist,
                                        (lsdata_t *)pent);
}

const lsbelist_t *lsbelist_unshift(const lsbelist_t *belist,
                                   lsbind_entry_t *ent) {
  return (const lsbelist_t *)lslist_unshift((const lslist_t *)belist, ent);
}

const lsbelist_t *lsbelist_shift(const lsbelist_t *belist,
                                 lsbind_entry_t **pent) {
  return (const lsbelist_t *)lslist_shift((const lslist_t *)belist,
                                          (lsdata_t *)pent);
}

lssize_t lsbelist_count(const lsbelist_t *belist) {
  return lslist_count((const lslist_t *)belist);
}

lsbind_entry_t *lsbelist_get(const lsbelist_t *belist, lssize_t i) {
  return (lsbind_entry_t *)lslist_get((const lslist_t *)belist, i);
}

const lsbelist_t *lsbelist_concat(const lsbelist_t *belist1,
                                  const lsbelist_t *belist2) {
  return (const lsbelist_t *)lslist_concat((const lslist_t *)belist1,
                                           (const lslist_t *)belist2);
}

const lsbelist_t *lsbelist_get_next(const lsbelist_t *belist) {
  return (const lsbelist_t *)lslist_get_next((const lslist_t *)belist);
}
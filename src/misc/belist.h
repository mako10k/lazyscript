#pragma once

typedef struct lsbelist lsbelist_t;

#include "misc/bind.h"

const lsbelist_t *lsbelist_new(void);
const lsbelist_t *lsbelist_push(const lsbelist_t *belist,
                                const lsbind_entry_t *ent);
const lsbelist_t *lsbelist_pop(const lsbelist_t *belist,
                               const lsbind_entry_t **pent);
const lsbelist_t *lsbelist_unshift(const lsbelist_t *belist,
                                   const lsbind_entry_t *ent);
const lsbelist_t *lsbelist_shift(const lsbelist_t *belist,
                                 const lsbind_entry_t **pent);
lssize_t lsbelist_count(const lsbelist_t *belist);
const lsbind_entry_t *lsbelist_get(const lsbelist_t *belist, lssize_t i);
const lsbelist_t *lsbelist_concat(const lsbelist_t *belist1,
                                  const lsbelist_t *belist2);
const lsbelist_t *lsbelist_get_next(const lsbelist_t *belist);

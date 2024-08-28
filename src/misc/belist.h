#pragma once

typedef struct lsbelist lsbelist_t;
typedef struct lsbelistm lsbelistm_t;

#include "expr/expr.h"

lsbelistm_t *lsbelistm_new(void);
void lsbelistm_push(lsbelistm_t **pbelistm, lsbind_entry_t *expr);
lsbind_entry_t *lsbelistm_pop(lsbelistm_t **pbelistm);
void lsbelistm_unshift(lsbelistm_t **pbelistm, lsbind_entry_t *expr);
lsbind_entry_t *lsbelistm_shift(lsbelistm_t **pbelistm);
lssize_t lsbelistm_count(const lsbelistm_t *belistm);
lsbind_entry_t *lsbelistm_get(const lsbelistm_t *belist, lssize_t i);
lsbelistm_t *lsbelistm_clone(const lsbelistm_t *belistm);
void lsbelistm_concat(lsbelistm_t **pbelistm, lsbelist_t *belistm);

const lsbelist_t *lsbelist_new(void);
const lsbelist_t *lsbelist_push(const lsbelist_t *belist, lsbind_entry_t *expr);
const lsbelist_t *lsbelist_pop(const lsbelist_t *belist,
                               lsbind_entry_t **pexpr);
const lsbelist_t *lsbelist_unshift(const lsbelist_t *belist,
                                   lsbind_entry_t *expr);
const lsbelist_t *lsbelist_shift(const lsbelist_t *belist,
                                 lsbind_entry_t **pexpr);
lssize_t lsbelist_count(const lsbelist_t *belist);
lsbind_entry_t *lsbelist_get(const lsbelist_t *belist, lssize_t i);
const lsbelist_t *lsbelist_concat(const lsbelist_t *belist1,
                                  const lsbelist_t *belist2);
const lsbelist_t *lsbelist_get_next(const lsbelist_t *belist);

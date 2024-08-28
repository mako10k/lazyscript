#pragma once

typedef struct lselist lselist_t;

#include "expr/expr.h"

const lselist_t *lselist_new(void);
const lselist_t *lselist_push(const lselist_t *elist, lsexpr_t *expr);
const lselist_t *lselist_pop(const lselist_t *elist, lsexpr_t **pexpr);
const lselist_t *lselist_unshift(const lselist_t *elist, lsexpr_t *expr);
const lselist_t *lselist_shift(const lselist_t *elist, lsexpr_t **pexpr);
lssize_t lselist_count(const lselist_t *elist);
lsexpr_t *lselist_get(const lselist_t *elist, lssize_t i);
const lselist_t *lselist_concat(const lselist_t *elist1,
                                const lselist_t *elist2);
const lselist_t *lselist_get_next(const lselist_t *elist);

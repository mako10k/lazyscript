#pragma once

typedef struct lselist lselist_t;
typedef struct lselistm lselistm_t;

#include "expr/expr.h"

lselistm_t *lselistm_new(void);
void lselistm_push(lselistm_t **pelistm, lsexpr_t *expr);
lsexpr_t *lselistm_pop(lselistm_t **pelistm);
void lselistm_unshift(lselistm_t **pelistm, lsexpr_t *expr);
lsexpr_t *lselistm_shift(lselistm_t **pelistm);
lssize_t lselistm_count(const lselistm_t *elistm);
lsexpr_t *lselistm_get(const lselistm_t *elist, lssize_t i);
lselistm_t *lselistm_clone(const lselistm_t *elistm);
void lselistm_concat(lselistm_t **pelistm, lselist_t *elistm);

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

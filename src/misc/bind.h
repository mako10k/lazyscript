#pragma once

typedef struct lsbind lsbind_t;
typedef struct lsbind_entry lsbind_entry_t;

#include "expr/expr.h"
#include "misc/belist.h"
#include "pat/pat.h"

const lsbind_entry_t *lsbind_entry_new(const lspat_t *lhs, const lsexpr_t *rhs);
void lsbind_entry_print(FILE *fp, lsprec_t prec, int indent,
                        const lsbind_entry_t *ent);
const lspat_t *lsbind_entry_get_lhs(const lsbind_entry_t *ent);
const lsexpr_t *lsbind_entry_get_rhs(const lsbind_entry_t *ent);

lsbind_t *lsbind_new(void);
void lsbind_push(lsbind_t *bind, const lsbind_entry_t *ent);
const lsbelist_t *lsbind_get_entries(const lsbind_t *bind);
lssize_t lsbind_get_entry_count(const lsbind_t *bind);
const lsbind_entry_t *lsbind_get_entry(const lsbind_t *bind, lssize_t i);
void lsbind_print(FILE *fp, lsprec_t prec, int indent, const lsbind_t *bind);

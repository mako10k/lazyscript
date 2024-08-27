#pragma once

typedef struct lseref lseref_t;

#include "eenv.h"
#include "erref.h"
#include "loc.h"
#include "str.h"

lseref_t *lseref_new(const lsstr_t *name, lsloc_t loc);
const lsstr_t *lseref_get_name(const lseref_t *eref);
void lseref_set_erref(lseref_t *eref, lserref_t *erref);
lserref_t *lseref_get_erref(const lseref_t *eref);
lsloc_t lseref_get_loc(const lseref_t *eref);
void lseref_print(FILE *fp, lsprec_t prec, int indent, const lseref_t *eref);
lspres_t lseref_prepare(lseref_t *eref, lseenv_t *env);
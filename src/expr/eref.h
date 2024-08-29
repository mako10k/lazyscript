#pragma once

typedef struct lseref lseref_t;

#include "common/loc.h"
#include "common/str.h"
#include "expr/eenv.h"
#include "expr/erref.h"

const lseref_t *lseref_new(const lsstr_t *name, lsloc_t loc);
const lsstr_t *lseref_get_name(const lseref_t *eref);
void lseref_set_erref(lseref_t *eref, const lserref_t *erref); // TODO: fix
const lserref_t *lseref_get_erref(const lseref_t *eref);
lsloc_t lseref_get_loc(const lseref_t *eref);
void lseref_print(FILE *fp, lsprec_t prec, int indent, const lseref_t *eref);
lspres_t lseref_prepare(const lseref_t *eref, lseenv_t *env);
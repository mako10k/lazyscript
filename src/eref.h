#pragma once

typedef struct lseref lseref_t;

#include "eenv.h"
#include "erref.h"
#include "loc.h"
#include "str.h"
#include "thunk.h"

lseref_t *lseref(const lsstr_t *name, lsloc_t loc);
const lsstr_t *lseref_get_name(const lseref_t *eref);
void lseref_set_erref(lseref_t *eref, lserref_t *erref);
lserref_t *lseref_get_erref(const lseref_t *eref);
lsloc_t lseref_get_loc(const lseref_t *eref);
void lseref_print(FILE *fp, int prec, int indent, const lseref_t *eref);
int lseref_prepare(lseref_t *eref, lseenv_t *env);
lsthunk_t *lseref_thunk(const lseref_t *eref);

#pragma once

typedef struct lspref lspref_t;

#include "eenv.h"
#include "lazyscript.h"
#include "loc.h"
#include "str.h"

lspref_t *lspref_new(const lsstr_t *name, lsloc_t loc);
const lsstr_t *lspref_get_name(const lspref_t *pref);
lsloc_t lspref_get_loc(const lspref_t *pref);
void lspref_print(FILE *fp, lsprec_t prec, int indent, const lspref_t *pref);
lspres_t lspref_prepare(lspref_t *pref, lseenv_t *eenv, lserref_wrapper_t *erref);
lsmres_t lspref_match(lstenv_t *tenv, const lspref_t *pref, lsthunk_t *thunk);
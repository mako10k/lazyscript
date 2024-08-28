#pragma once

#include <stdio.h>

typedef struct lspas lspas_t;

#include "common/str.h"
#include "pat/pat.h"
#include "pat/pref.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"

lspas_t *lspas_new(lspref_t *pref, lspat_t *pat);
void lspas_print(FILE *fp, int prec, int indent, lspas_t *pas);
lspres_t lspas_prepare(lspas_t *pas, lseenv_t *env, lserref_wrapper_t *erref);
lsmres_t lspas_match(lstenv_t *tenv, const lspas_t *pas, lsthunk_t *thunk);
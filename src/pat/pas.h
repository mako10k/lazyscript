#pragma once

#include <stdio.h>

typedef struct lspas lspas_t;

#include "common/str.h"
#include "expr/erref.h"
#include "pat/pat.h"
#include "pat/pref.h"

const lspas_t *lspas_new(const lspref_t *pref, const lspat_t *pat);
void lspas_print(FILE *fp, lsprec_t prec, int indent, const lspas_t *pas);
lspres_t lspas_prepare(const lspas_t *pas, lseenv_t *env,
                       const lserref_base_t *erref);
const lspref_t *lspas_get_pref(const lspas_t *pas);
const lspat_t *lspas_get_pat(const lspas_t *pas);

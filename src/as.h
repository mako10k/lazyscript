#pragma once

#include <stdio.h>

typedef struct lsas lsas_t;

#include "pat.h"
#include "pref.h"
#include "str.h"

lsas_t *lsas(lspref_t *pref, lspat_t *pat);
void lsas_print(FILE *fp, int prec, int indent, lsas_t *as);
int lsas_prepare(lsas_t *as, lsenv_t *env, lserref_t *erref);
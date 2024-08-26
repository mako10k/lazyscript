#pragma once

#include <stdio.h>

typedef struct lspas lspas_t;

#include "pat.h"
#include "pref.h"
#include "str.h"

lspas_t *lspas(lspref_t *pref, lspat_t *pat);
void lspas_print(FILE *fp, int prec, int indent, lspas_t *as);
int lspas_prepare(lspas_t *as, lseenv_t *env, lserref_wrapper_t *erref);
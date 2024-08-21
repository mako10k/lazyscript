#pragma once

#include <stdio.h>

typedef struct lsas lsas_t;

#include "pat.h"
#include "pref.h"
#include "str.h"

lsas_t *lsas(lspref_t *pref, const lspat_t *pat);
void lsas_print(FILE *fp, int prec, int indent, const lsas_t *as);
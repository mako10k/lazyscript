#pragma once

#include <stdio.h>

typedef struct lspas lspas_t;

#include "common/str.h"
#include "pat/pat.h"

const lspas_t *lspas_new(const lsref_t *ref, const lspat_t *pat);
const lsref_t *lspas_get_ref(const lspas_t *pas);
const lspat_t *lspas_get_pat(const lspas_t *pas);
void lspas_print(FILE *fp, lsprec_t prec, int indent, const lspas_t *pas);

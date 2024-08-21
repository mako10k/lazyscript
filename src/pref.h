#pragma once

typedef struct lspref lspref_t;

#include "str.h"

lspref_t *lspref(const lsstr_t *name);
const lsstr_t *lspref_get_name(const lspref_t *ref);
void lspref_print(FILE *fp, int prec, int indent, const lspref_t *ref);
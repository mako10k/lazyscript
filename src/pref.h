#pragma once

typedef struct lspref lspref_t;

#include "eenv.h"
#include "lazyscript.h"
#include "loc.h"
#include "str.h"

lspref_t *lspref(const lsstr_t *name, lsloc_t loc);
const lsstr_t *lspref_get_name(const lspref_t *ref);
lsloc_t lspref_get_loc(const lspref_t *ref);
void lspref_print(FILE *fp, int prec, int indent, const lspref_t *ref);
int lspref_prepare(lspref_t *ref, lseenv_t *env, lserref_wrapper_t *erref);
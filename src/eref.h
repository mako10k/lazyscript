#pragma once

typedef struct lseref lseref_t;

#include "env.h"
#include "erref.h"
#include "str.h"

lseref_t *lseref(const lsstr_t *name);
const lsstr_t *lseref_get_name(const lseref_t *eref);
void lseref_set_erref(lseref_t *eref, lserref_t *erref);
lserref_t *lseref_get_erref(const lseref_t *eref);
void lseref_print(FILE *fp, int prec, int indent, const lseref_t *eref);
int lseref_prepare(lseref_t *eref, lsenv_t *env);
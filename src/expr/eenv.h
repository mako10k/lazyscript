#pragma once

typedef struct lseenv lseenv_t;

#include "common/str.h"
#include "expr/erref.h"

lseenv_t *lseenv_new(const lseenv_t *parent);
lserref_t *lseenv_get(const lseenv_t *eenv, const lsstr_t *name);
lserref_t *lseenv_get_self(const lseenv_t *eenv, const lsstr_t *name);
void lseenv_put(lseenv_t *eenv, const lsstr_t *name, const lserref_t *erref);
void lseenv_incr_nwarnings(lseenv_t *eenv);
void lseenv_incr_nerrors(lseenv_t *eenv);
void lseenv_incr_nfatals(lseenv_t *eenv);
lssize_t lseenv_get_nwarnings(const lseenv_t *eenv);
lssize_t lseenv_get_nerrors(const lseenv_t *eenv);
lssize_t lseenv_get_nfatals(const lseenv_t *eenv);
void lseenv_print(FILE *fp, const lseenv_t *eenv);

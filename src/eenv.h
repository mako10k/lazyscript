#pragma once

typedef struct lseenv lseenv_t;

#include "erref.h"
#include "str.h"

lseenv_t *lseenv(lseenv_t *parent);
lserref_t *lseenv_get(const lseenv_t *env, const lsstr_t *name);
lserref_t *lseenv_get_self(const lseenv_t *env, const lsstr_t *name);
void lseenv_put(lseenv_t *env, const lsstr_t *name, lserref_t *erref);

void lseenv_incr_nwarnings(lseenv_t *env);
void lseenv_incr_nerrors(lseenv_t *env);
void lseenv_incr_nfatals(lseenv_t *env);
int lseenv_get_nwarnings(const lseenv_t *env);
int lseenv_get_nerrors(const lseenv_t *env);
int lseenv_get_nfatals(const lseenv_t *env);
void lseenv_print(FILE *fp, const lseenv_t *env);

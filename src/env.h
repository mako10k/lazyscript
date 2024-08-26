#pragma once

typedef struct lsenv lsenv_t;

#include "erref.h"
#include "str.h"

lsenv_t *lsenv(lsenv_t *parent);
lserref_t *lsenv_get(const lsenv_t *env, const lsstr_t *name);
lserref_t *lsenv_get_self(const lsenv_t *env, const lsstr_t *name);
void lsenv_put(lsenv_t *env, const lsstr_t *name, lserref_t *erref);

void lsenv_incr_nwarnings(lsenv_t *env);
void lsenv_incr_nerrors(lsenv_t *env);
void lsenv_incr_nfatals(lsenv_t *env);
int lsenv_get_nwarnings(const lsenv_t *env);
int lsenv_get_nerrors(const lsenv_t *env);
int lsenv_get_nfatals(const lsenv_t *env);
void lsenv_print(FILE *fp, const lsenv_t *env);

#pragma once

typedef struct lsenv lsenv_t;

#include "str.h"
#include "erref.h"

lsenv_t *lsenv(const lsenv_t *parent);
lserref_t *lsenv_get(const lsenv_t *env, const lsstr_t *name);
lserref_t *lsenv_get_self(const lsenv_t *env, const lsstr_t *name);
void lsenv_put(lsenv_t *env, const lsstr_t *name, lserref_t *erref);
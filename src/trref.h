#pragma once

typedef struct lstrref lstrref_t;

#include "erref.h"
#include "str.h"
#include "tenv.h"

lstrref_t *lstrref(lstenv_t *tenv, const lsstr_t *name, const lserref_t *erref);
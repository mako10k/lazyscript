#pragma once

typedef struct lstrref lstrref_t;

#include "erref.h"
#include "str.h"
#include "tenv.h"
#include "thunk.h"

lstrref_t *lstrref_new(lstenv_t *tenv, const lsstr_t *name, const lserref_t *erref);
lsthunk_t *lstrref_eval(lstrref_t *trref);
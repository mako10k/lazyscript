#pragma once

typedef struct lstrref lstrref_t;

#include "common/str.h"
#include "expr/erref.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"

lstrref_t *lstrref_new(lstenv_t *tenv, const lsstr_t *name,
                       const lserref_t *erref);
lsthunk_t *lstrref_eval(lstrref_t *trref);
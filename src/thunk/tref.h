#pragma once

typedef struct lstref lstref_t;

#include "expr/eref.h"
#include "thunk/thunk.h"

lstref_t *lstref_new(const lseref_t *eref);
lsthunk_t *lstref_eval(lstref_t *tref);
lsthunk_t *lstref_apply(lstref_t *tref, const lstlist_t *args);
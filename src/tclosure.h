#pragma once

typedef struct lstclosure lstclosure_t;

#include "eclosure.h"
#include "tenv.h"
#include "thunk.h"
#include "tlist.h"

lstclosure_t *lstclosure_new(lstenv_t *tenv, const lseclosure_t *eclosure);
lsthunk_t *lstclosure_eval(lstclosure_t *tclosure);
lsthunk_t *lstclosure_apply(lstclosure_t *tclosure, const lstlist_t *args);
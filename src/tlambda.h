#pragma once

typedef struct lstlambda lstlambda_t;

#include "elambda.h"
#include "tenv.h"
#include "thunk.h"
#include "tlist.h"

lstlambda_t *lstlambda_new(lstenv_t *tenv, const lselambda_t *elambda);
lsthunk_t *lstlambda_apply(lstlambda_t *tlambda, const lstlist_t *args);
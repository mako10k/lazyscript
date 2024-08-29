#pragma once

typedef struct lstlambda lstlambda_t;

#include "expr/elambda.h"
#include "thunk/thunk.h"
#include "thunk/tlist.h"
#include "thunk/tenv.h"

lstlambda_t *lstlambda_new(const lselambda_t *elambda, lstenv_t *tenv);
lsthunk_t *lstlambda_apply(lstlambda_t *tlambda, const lstlist_t *args);
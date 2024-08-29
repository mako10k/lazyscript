#pragma once

typedef struct lstlambda lstlambda_t;

#include "expr/elambda.h"
#include "thunk/thunk.h"
#include "thunk/tlist.h"

lstlambda_t *lstlambda_new(const lselambda_t *elambda);
lsthunk_t *lstlambda_apply(lstlambda_t *tlambda, const lstlist_t *args);
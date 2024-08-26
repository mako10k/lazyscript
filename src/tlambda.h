#pragma once

typedef struct lstlambda lstlambda_t;

#include "elambda.h"
#include "tenv.h"

lstlambda_t *lstlambda(lstenv_t *tenv, const lselambda_t *elambda);
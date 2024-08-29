#pragma once

typedef struct lstenv lstenv_t;

#include "thunk/thunk.h"

lstenv_t *lstenv_new(lstenv_t *parent);
lsthunk_t *lstenv_get(const lstenv_t *env, const lsstr_t *name);
void lstenv_put(lstenv_t *env, const lsstr_t *name, lsthunk_t *thunk);
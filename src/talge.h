#pragma once

typedef struct lstalge lstalge_t;

#include "ealge.h"
#include "tenv.h"
#include "thunk.h"

lstalge_t *lstalge(lstenv_t *tenv, const lsealge_t *ealge);
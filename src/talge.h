#pragma once

typedef struct lstalge lstalge_t;

#include "ealge.h"
#include "tenv.h"
#include "thunk.h"
#include "tlist.h"

lstalge_t *lstalge_new(lstenv_t *tenv, const lsealge_t *ealge);
const lsstr_t *lstalge_get_constr(const lstalge_t *talge);
lssize_t lstalge_get_argc(const lstalge_t *talge);
lsthunk_t *lstalge_get_arg(const lstalge_t *talge, int i);
const lstlist_t *lstalge_get_args(const lstalge_t *talge);
lsthunk_t *lstalge_apply(lstalge_t *talge, const lstlist_t *args);
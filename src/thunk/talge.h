#pragma once

typedef struct lstalge lstalge_t;

#include "expr/ealge.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"
#include "thunk/tlist.h"

lstalge_t *lstalge_new(lstenv_t *tenv, const lsealge_t *ealge);
const lsstr_t *lstalge_get_constr(const lstalge_t *talge);
lssize_t lstalge_get_arg_count(const lstalge_t *talge);
lsthunk_t *lstalge_get_arg(const lstalge_t *talge, int i);
const lstlist_t *lstalge_get_args(const lstalge_t *talge);
lsthunk_t *lstalge_apply(lstalge_t *talge, const lstlist_t *args);
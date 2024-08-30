#pragma once

typedef struct lstalge lstalge_t;

#include "expr/ealge.h"
#include "thunk/tenv.h"
#include "thunk/tlist.h"

lstalge_t *lstalge_new(const lsealge_t *ealge, lstenv_t *tenv);
const lsstr_t *lstalge_get_constr(const lstalge_t *talge);
lssize_t lstalge_get_arg_count(const lstalge_t *talge);
lsthunk_t *lstalge_get_arg(const lstalge_t *talge, int i);
const lstlist_t *lstalge_get_args(const lstalge_t *talge);
lsthunk_t *lstalge_apply(lstalge_t *talge, size_t argc,
                         const lsthunk_t *args[]);
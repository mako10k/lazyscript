#pragma once

#include "lstypes.h"
typedef struct lstpat lstpat_t;

#include "common/int.h"
#include "common/str.h"
#include "thunk/tenv.h"

lstpat_t *lstpat_new_alge(const lsstr_t *constr, lssize_t argc,
                          lstpat_t *const *args);

lstpat_t *lstpat_new_ref(const lsref_t *ref);

lstpat_t *lstpat_new_as(const lsref_t *ref, lstpat_t *aspattern);

lstpat_t *lstpat_new_int(const lsint_t *intval);

lstpat_t *lstpat_new_str(const lsstr_t *strval);

lstpat_t *lstpat_new_pat(const lspat_t *pat, lstenv_t *env);

lsptype_t lstpat_get_type(const lstpat_t *pat);

const lsstr_t *lstpat_get_constr(const lstpat_t *pat);

lssize_t lstpat_get_argc(const lstpat_t *pat);

lstpat_t *const *lstpat_get_args(const lstpat_t *pat);

lstpat_t *lstpat_get_ref(const lstpat_t *pat);

void lstpat_set_reftarget(lstpat_t *pat, lsthunk_t *target);

lstpat_t *lstpat_get_aspattern(const lstpat_t *pat);

const lsint_t *lstpat_get_int(const lstpat_t *pat);

const lsstr_t *lstpat_get_str(const lstpat_t *pat);
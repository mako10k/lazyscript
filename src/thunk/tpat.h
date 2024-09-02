#pragma once

typedef struct lstpat lstpat_t;

#include "common/int.h"
#include "common/str.h"
#include "thunk/tenv.h"

lstpat_t *lstpat_new_alge(const lsstr_t *constr, lssize_t argc,
                          lstpat_t *const *args);

lstpat_t *lstpat_new_ref(const lsstr_t *refname);

lstpat_t *lstpat_new_as(const lsstr_t *refname, lstpat_t *aspattern);

lstpat_t *lstpat_new_int(const lsint_t *intval);

lstpat_t *lstpat_new_str(const lsstr_t *strval);

lstpat_t *lstpat_new_pat(const lspat_t *pat, lstenv_t *env);
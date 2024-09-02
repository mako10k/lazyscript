#pragma once

typedef struct lstenv lstenv_t;

#include "common/str.h"
#include "thunk/thunk.h"

lstenv_t *lstenv_new(const lstenv_t *parent);
lstref_target_t *lstenv_get(const lstenv_t *tenv, const lsstr_t *name);
lstref_target_t *lstenv_get_self(const lstenv_t *tenv, const lsstr_t *name);
void lstenv_put(lstenv_t *tenv, const lsstr_t *name,  lstref_target_t *target);
void lstenv_incr_nwarnings(lstenv_t *tenv);
void lstenv_incr_nerrors(lstenv_t *tenv);
void lstenv_incr_nfatals(lstenv_t *tenv);
lssize_t lstenv_get_nwarnings(const lstenv_t *tenv);
lssize_t lstenv_get_nerrors(const lstenv_t *tenv);
lssize_t lstenv_get_nfatals(const lstenv_t *tenv);
void lstenv_print(FILE *fp, const lstenv_t *tenv);

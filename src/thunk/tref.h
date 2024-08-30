#pragma once

typedef struct lstref lstref_t;
typedef struct lstref_entry lstref_entry_t;
typedef struct lstref_target lstref_target_t;
typedef struct lstref_bind lstref_bind_t;
typedef struct lstref_lambda lstref_lambda_t;
typedef enum lstrtype {
  LSTRTYPE_bind,
  LSTRTYPE_LAMBDA,
  LSTRTYPE_THUNK
} lstrtype_t;

#include "common/ref.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"

lstref_t *lstref_new(const lsref_t *ref, lstenv_t *tenv);

lstref_t *lstref_new_bind(const lsbind_t *bentry, lstenv_t *tenv);

lstref_t *lstref_new_thunk(const lsref_t *ref, lsthunk_t *thunk);
lsthunk_t *lstref_eval(lstref_t *tref);
lsthunk_t *lstref_apply(lstref_t *tref, const lstlist_t *args);

const lstref_target_t *lstref_target_new_lambda(const lselambda_t *elambda,
                                                lstenv_t *tenv);
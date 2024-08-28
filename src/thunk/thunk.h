#pragma once

typedef struct lsthunk lsthunk_t;

#include "common/int.h"
#include "common/str.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pat.h"
#include "pat/pref.h"
#include "thunk/talge.h"
#include "thunk/tappl.h"
#include "thunk/tclosure.h"
#include "thunk/tenv.h"
#include "thunk/tlambda.h"
#include "thunk/tref.h"

typedef enum lsttype {
  LSTTYPE_ALGE,
  LSTTYPE_APPL,
  LSTTYPE_INT,
  LSTTYPE_STR,
  LSTTYPE_CLOSURE,
  LSTTYPE_LAMBDA,
  LSTTYPE_REF
} lsttype_t;

lsthunk_t *lsthunk_new_int(const lsint_t *intval);
lsthunk_t *lsthunk_new_str(const lsstr_t *strval);
lsthunk_t *lsthunk_new_appl(lstappl_t *tappl);
lsthunk_t *lsthunk_new_alge(lstalge_t *talge);
lsthunk_t *lsthunk_new_lambda(lstlambda_t *tlambda);
lsthunk_t *lsthunk_new_closure(lstclosure_t *tclosure);
lsthunk_t *lsthunk_new_ref(lstref_t *tref);
lsthunk_t *lsthunk_new_expr(lstenv_t *tenv, const lsexpr_t *expr);

lsthunk_t *lsthunk_get_whnf(lsthunk_t *thunk);
lsttype_t lsthunk_get_type(const lsthunk_t *thunk);

lstalge_t *lsthunk_get_alge(const lsthunk_t *thunk);
lstappl_t *lsthunk_get_appl(const lsthunk_t *thunk);
const lsint_t *lsthunk_get_int(const lsthunk_t *thunk);
const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk);
lstclosure_t *lsthunk_get_closure(const lsthunk_t *thunk);
lstlambda_t *lsthunk_get_lambda(const lsthunk_t *thunk);
lstref_t *lsthunk_get_ref(const lsthunk_t *thunk);

lsmres_t lsthunk_match_pat(lstenv_t *tenv, const lspat_t *pat,
                           lsthunk_t *thunk);
lsmres_t lsthunk_match_pat_alge(lstenv_t *tenv, const lspalge_t *alge,
                                lsthunk_t *thunk);
lsmres_t lsthunk_match_pat_as(lstenv_t *tenv, const lspas_t *pas,
                              lsthunk_t *thunk);
lsmres_t lsthunk_match_pat_ref(lstenv_t *tenv, const lspref_t *pref,
                               lsthunk_t *thunk);
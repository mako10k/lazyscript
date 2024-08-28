#include "thunk/thunk.h"
#include "common/malloc.h"
#include "thunk/talge.h"
#include "thunk/tclosure.h"
#include "thunk/tlambda.h"
#include <assert.h>

struct lsthunk {
  lsttype_t lt_type;
  union {
    lstalge_t *lt_alge;
    lstappl_t *lt_appl;
    const lsint_t *lt_int;
    const lsstr_t *lt_str;
    lstref_t *lt_ref;
    lstlambda_t *lt_lambda;
    lstclosure_t *lt_closure;
  };
  lsthunk_t *lt_whnf;
};

lsthunk_t *lsthunk_appl(lstappl_t *tappl) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_APPL;
  thunk->lt_appl = tappl;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_alge(lstalge_t *talge) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_ALGE;
  thunk->lt_alge = talge;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_int(const lsint_t *intval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_INT;
  thunk->lt_int = intval;
  thunk->lt_whnf = thunk;
  return thunk;
}

lsthunk_t *lsthunk_str(const lsstr_t *strval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_STR;
  thunk->lt_str = strval;
  thunk->lt_whnf = thunk;
  return thunk;
}

lsthunk_t *lsthunk_ref(lstref_t *tref) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_REF;
  thunk->lt_ref = tref;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_lambda(lstlambda_t *tlambda) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_LAMBDA;
  thunk->lt_lambda = tlambda;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_closure(lstclosure_t *tclosure) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_CLOSURE;
  thunk->lt_closure = tclosure;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_get_whnf(lsthunk_t *thunk) {
  assert(thunk != NULL);
  if (thunk->lt_whnf != NULL)
    return thunk->lt_whnf;

  switch (thunk->lt_type) {
  case LSTTYPE_APPL:
    thunk->lt_whnf = lstappl_eval(thunk->lt_appl);
    break;
  case LSTTYPE_CLOSURE:
    thunk->lt_whnf = lstclosure_eval(thunk->lt_closure);
    break;
  case LSTTYPE_REF:
    thunk->lt_whnf = lstref_eval(thunk->lt_ref);
    break;
  default:
    // it already is in WHNF
    thunk->lt_whnf = thunk;
    break;
  }
  return thunk->lt_whnf;
}

lsttype_t lsthunk_type(const lsthunk_t *thunk) { return thunk->lt_type; }

lstalge_t *lsthunk_get_alge(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge;
}
lstappl_t *lsthunk_get_appl(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_appl;
}
const lsint_t *lsthunk_get_int(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}
const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}
lstclosure_t *lsthunk_get_closure(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_CLOSURE);
  return thunk->lt_closure;
}
lstlambda_t *lsthunk_get_lambda(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda;
}
lstref_t *lsthunk_get_ref(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_REF);
  return thunk->lt_ref;
}

lsthunk_t *lsthunk_new_expr(lstenv_t *tenv, const lsexpr_t *expr) {
  assert(expr != NULL);
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_ALGE:
    return lsthunk_alge(lstalge_new(tenv, lsexpr_get_alge(expr)));
  case LSETYPE_APPL:
    return lsthunk_appl(lstappl_new(tenv, lsexpr_get_appl(expr)));
  case LSETYPE_REF:
    return lsthunk_ref(lstref_new(tenv, lsexpr_get_ref(expr)));
  case LSETYPE_LAMBDA:
    return lsthunk_lambda(lstlambda_new(tenv, lsexpr_get_lambda(expr)));
  case LSETYPE_CLOSURE:
    return lsthunk_closure(lstclosure_new(tenv, lsexpr_get_closure(expr)));
  case LSETYPE_INT:
    return lsthunk_int(lsexpr_get_int(expr));
  case LSETYPE_STR:
    return lsthunk_str(lsexpr_get_str(expr));
  }
  assert(0);
}

static lsmres_t lsthunk_match_int(lstenv_t *tenv, const lsint_t *intval,
                                  lsthunk_t *thunk) {
  (void)tenv;
  lsttype_t ttype = lsthunk_type(thunk);
  if (ttype != LSTTYPE_INT)
    return LSMATCH_FAILURE;
  const lsint_t *tintval = lsthunk_get_int(thunk);
  return lsint_eq(intval, tintval) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

static lsmres_t lsthunk_match_str(lstenv_t *tenv, const lsstr_t *strval,
                                  lsthunk_t *thunk) {
  (void)tenv;
  lsttype_t ttype = lsthunk_type(thunk);
  if (ttype != LSTTYPE_STR)
    return LSMATCH_FAILURE;
  const lsstr_t *tstrval = lsthunk_get_str(thunk);
  return lsstrcmp(strval, tstrval) == 0 ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_pat(lstenv_t *tenv, const lspat_t *pat,
                           lsthunk_t *thunk) {
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE:
    return lsthunk_match_alge(tenv, lspat_get_alge(pat), thunk);
  case LSPTYPE_AS:
    return lsthunk_match_as(tenv, lspat_get_as(pat), thunk);
  case LSPTYPE_INT:
    return lsthunk_match_int(tenv, lspat_get_int(pat), thunk);
  case LSPTYPE_STR:
    return lsthunk_match_str(tenv, lspat_get_str(pat), thunk);
  case LSPTYPE_REF:
    return lsthunk_match_ref(tenv, lspat_get_ref(pat), thunk);
  }
  return LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_alge(lstenv_t *tenv, const lspalge_t *palge,
                            lsthunk_t *thunk) {
  assert(palge != NULL);
  lsthunk_t *thunk_whnf = lsthunk_get_whnf(thunk);
  lsttype_t ttype = lsthunk_type(thunk_whnf);
  if (ttype != LSTTYPE_ALGE)
    return LSMATCH_FAILURE;
  lstalge_t *talge = lsthunk_get_alge(thunk_whnf);
  const lsstr_t *tconstr = lstalge_get_constr(talge);
  const lsstr_t *pconstr = lspalge_get_constr(palge);
  if (lsstrcmp(pconstr, tconstr) != 0)
    return LSMATCH_FAILURE;
  const lsplist_t *pargs = lspalge_get_args(palge);
  lssize_t pargc = lspalge_get_arg_count(palge);
  lssize_t targc = lstalge_get_arg_count(talge);
  if (pargc != targc)
    return LSMATCH_FAILURE;
  for (lssize_t i = 0; i < pargc; i++) {
    lspat_t *parg = lsplist_get(pargs, i);
    lsthunk_t *targ = lstalge_get_arg(talge, i);
    if (lsthunk_match_pat(tenv, parg, targ) < 0)
      return LSMATCH_FAILURE;
  }
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_as(lstenv_t *tenv, const lspas_t *pas,
                          lsthunk_t *thunk) {
  lspref_t *pref = lspas_get_pref(pas);
  lspat_t *pat = lspas_get_pat(pas);
  lsmres_t mres = lsthunk_match_pat(tenv, pat, thunk);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  mres = lsthunk_match_ref(tenv, pref, thunk);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_ref(lstenv_t *tenv, const lspref_t *pref,
                           lsthunk_t *thunk) {
  return LSMATCH_FAILURE; // TODO: implement
}
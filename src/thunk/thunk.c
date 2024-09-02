#include "thunk/thunk.h"
#include "common/io.h"
#include "common/loc.h"
#include "common/malloc.h"
#include "expr/eclosure.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"
#include <assert.h>
#include <stddef.h>

struct lstalge {
  const lsstr_t *lta_constr;
  lssize_t lta_argc;
  lsthunk_t *lta_args[0];
};

struct lstappl {
  lsthunk_t *lta_func;
  lssize_t lta_argc;
  lsthunk_t *lta_args[0];
};

struct lstchoice {
  lsthunk_t *ltc_left;
  lsthunk_t *ltc_right;
};

struct lstbind {
  lstpat_t *ltb_lhs;
  lsthunk_t *ltb_rhs;
};

struct lstlambda {
  lstpat_t *ltl_param;
  lsthunk_t *ltl_body;
};

struct lstref_target {
  lstrtype_t lrt_type;
  union {
    lstbind_t lrt_bind;
    lstlambda_t lrt_lambda;
  };
};

struct lstref {
  const lsstr_t *ltr_refname;
  lsthunk_t *ltr_refthunk;
  lstref_target_t *ltr_target;
};

struct lsthunk {
  lsttype_t lt_type;
  lsthunk_t *lt_whnf;
  union {
    lstalge_t lt_alge;
    lstappl_t lt_appl;
    lstchoice_t lt_choice;
    lstlambda_t lt_lambda;
    lstref_t lt_ref;
    const lsint_t *lt_int;
    const lsstr_t *lt_str;
  };
};

lsthunk_t *lsthunk_new_ealge(const lsealge_t *ealge, lstenv_t *tenv) {
  lssize_t eargc = lsealge_get_argc(ealge);
  const lsexpr_t *const *eargs = lsealge_get_args(ealge);
  lsthunk_t *thunk =
      lsmalloc(lssizeof(lsthunk_t, lt_alge) + eargc * sizeof(lsthunk_t *));
  thunk->lt_type = LSTTYPE_ALGE;
  thunk->lt_whnf = thunk;
  thunk->lt_alge.lta_constr = lsealge_get_constr(ealge);
  thunk->lt_alge.lta_argc = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_alge.lta_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return thunk;
}

lsthunk_t *lsthunk_new_eappl(const lseappl_t *eappl, lstenv_t *tenv) {
  lssize_t eargc = lseappl_get_argc(eappl);
  const lsexpr_t *const *eargs = lseappl_get_args(eappl);
  lsthunk_t *thunk =
      lsmalloc(lssizeof(lsthunk_t, lt_appl) + eargc * sizeof(lsthunk_t *));
  thunk->lt_type = LSTTYPE_APPL;
  thunk->lt_whnf = NULL;
  thunk->lt_appl.lta_func = lsthunk_new_expr(lseappl_get_func(eappl), tenv);
  thunk->lt_appl.lta_argc = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_appl.lta_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return thunk;
}

lsthunk_t *lsthunk_new_echoice(const lsechoice_t *echoice, lstenv_t *tenv) {
  lsthunk_t *thunk = lsmalloc(offsetof(lsthunk_t, lt_choice));
  thunk->lt_type = LSTTYPE_CHOICE;
  thunk->lt_whnf = NULL;
  thunk->lt_choice.ltc_left =
      lsthunk_new_expr(lsechoice_get_left(echoice), tenv);
  thunk->lt_choice.ltc_right =
      lsthunk_new_expr(lsechoice_get_right(echoice), tenv);
  return thunk;
}

lsthunk_t *lsthunk_new_eclosure(const lseclosure_t *eclosure, lstenv_t *tenv) {
  tenv = lstenv_new(tenv);
  lssize_t ebindc = lseclosure_get_bindc(eclosure);
  const lsbind_t *const *ebinds = lseclosure_get_binds(eclosure);
  lstref_target_t *targets[ebindc];
  for (lssize_t i = 0; i < ebindc; i++) {
    const lspat_t *lhs = lsbind_get_lhs(ebinds[i]);
    targets[i] = lsmalloc(sizeof(lstref_target_t));
    targets[i]->lrt_type = LSTRTYPE_BIND;
    targets[i]->lrt_bind.ltb_lhs = lstpat_new_pat(lhs, tenv);
    if (targets[i]->lrt_bind.ltb_lhs == NULL)
      return NULL;
  }
  for (lssize_t i = 0; i < ebindc; i++) {
    const lsexpr_t *rhs = lsbind_get_rhs(ebinds[i]);
    targets[i]->lrt_bind.ltb_rhs = lsthunk_new_expr(rhs, tenv);
    if (targets[i]->lrt_bind.ltb_rhs == NULL)
      return NULL;
  }
  return lsthunk_new_expr(lseclosure_get_expr(eclosure), tenv);
}

lsthunk_t *lsthunk_new_ref(const lsref_t *ref, lstenv_t *tenv) {
  lstref_target_t *target = lstenv_get(tenv, lsref_get_name(ref));
  lsthunk_t *thunk = lsmalloc(lssizeof(lsthunk_t, lt_ref));
  thunk->lt_type = LSTTYPE_REF;
  thunk->lt_whnf = NULL;
  thunk->lt_ref.ltr_refname = lsref_get_name(ref);
  thunk->lt_ref.ltr_refthunk = NULL;
  thunk->lt_ref.ltr_target = target;
  if (target == NULL) {
    lsprintf(stderr, 0, "E: ");
    lsloc_print(stderr, lsref_get_loc(ref));
    lsprintf(stderr, 0, "undefined reference: %s\n", lsref_get_name(ref));
    lstenv_incr_nerrors(tenv);
  }
  return thunk;
}

lsthunk_t *lsthunk_new_int(const lsint_t *intval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_INT;
  thunk->lt_whnf = thunk;
  thunk->lt_int = intval;
  return thunk;
}

lsthunk_t *lsthunk_new_str(const lsstr_t *strval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_STR;
  thunk->lt_whnf = thunk;
  thunk->lt_str = strval;
  return thunk;
}

lsthunk_t *lsthunk_new_elambda(const lselambda_t *elambda, lstenv_t *tenv) {
  const lspat_t *pparam = lselambda_get_param(elambda);
  const lsexpr_t *ebody = lselambda_get_body(elambda);
  tenv = lstenv_new(tenv);
  lstpat_t *tparam = lstpat_new_pat(pparam, tenv);
  if (tparam == NULL)
    return NULL;
  lsthunk_t *tbody = lsthunk_new_expr(ebody, tenv);
  if (tbody == NULL)
    return NULL;
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_LAMBDA;
  thunk->lt_whnf = thunk;
  thunk->lt_lambda.ltl_param = tparam;
  thunk->lt_lambda.ltl_body = tbody;
  return thunk;
}

lsthunk_t *lsthunk_new_expr(const lsexpr_t *expr, lstenv_t *tenv) {
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_ALGE:
    return lsthunk_new_ealge(lsexpr_get_alge(expr), tenv);
  case LSETYPE_APPL:
    return lsthunk_new_eappl(lsexpr_get_appl(expr), tenv);
  case LSETYPE_CHOICE:
    return lsthunk_new_echoice(lsexpr_get_choice(expr), tenv);
  case LSETYPE_CLOSURE:
    return lsthunk_new_eclosure(lsexpr_get_closure(expr), tenv);
  case LSETYPE_LAMBDA:
    return lsthunk_new_elambda(lsexpr_get_lambda(expr), tenv);
  case LSETYPE_INT:
    return lsthunk_new_int(lsexpr_get_int(expr));
  case LSETYPE_REF:
    return lsthunk_new_ref(lsexpr_get_ref(expr), tenv);
  case LSETYPE_STR:
    return lsthunk_new_str(lsexpr_get_str(expr));
  }
  assert(0);
}

lsttype_t lsthunk_get_type(const lsthunk_t *thunk) {
  assert(thunk != NULL);
  return thunk->lt_type;
}

const lsstr_t *lsthunk_get_constr(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge.lta_constr;
}

lsthunk_t *lsthunk_get_func(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_appl.lta_func;
}

lssize_t lsthunk_get_argc(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_argc
                                        : thunk->lt_appl.lta_argc;
}

lsthunk_t *const *lsthunk_get_args(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_args
                                        : thunk->lt_appl.lta_args;
}

lsthunk_t *lsthunk_get_left(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice.ltc_left;
}

lsthunk_t *lsthunk_get_right(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice.ltc_right;
}

lstpat_t *lsthunk_get_param(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_param;
}

lsthunk_t *lsthunk_get_body(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_body;
}

lstref_target_t *lsthunk_get_ref_target(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_REF);
  return thunk->lt_ref.ltr_target;
}

const lsint_t *lsthunk_get_int(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}

const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}

lsmres_t lsthunk_match_palge(lsthunk_t *thunk, const lspalge_t *palge) {
  assert(palge != NULL);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
  if (ttype != LSTTYPE_ALGE)
    return LSMATCH_FAILURE;
  const lsstr_t *tconstr = lsthunk_get_constr(thunk_whnf);
  const lsstr_t *pconstr = lspalge_get_constr(palge);
  if (lsstrcmp(pconstr, tconstr) != 0)
    return LSMATCH_FAILURE;
  const lspat_t *pargs = lspalge_get_args(palge);
  lssize_t pargc = lspalge_get_arg_count(palge);
  lssize_t targc = lstalge_get_arg_count(talge);
  if (pargc != targc)
    return LSMATCH_FAILURE;
  for (lssize_t i = 0; i < pargc; i++) {
    const lspat_t *parg = lsplist_get(pargs, i);
    lsthunk_t *targ = lstalge_get_arg(talge, i);
    if (lsthunk_match_pat(targ, parg, tenv) < 0)
      return LSMATCH_FAILURE;
  }
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pas(lsthunk_t *thunk, const lspas_t *pas,
                           lstenv_t *tenv) {
  const lsref_t *ref = lspas_get_ref(pas);
  const lspat_t *pat = lspas_get_pat(pas);
  lsmres_t mres = lsthunk_match_pat(thunk, pat, tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  mres = lsthunk_match_pref(thunk, ref, tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return LSMATCH_SUCCESS;
}

/**
 * Match an integer value with a thunk
 * @param thunk The thunk
 * @param intval The integer value
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_int(lsthunk_t *thunk, const lsint_t *intval,
                                  lstenv_t *tenv) {
  (void)tenv;
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_INT)
    return LSMATCH_FAILURE;
  const lsint_t *tintval = lsthunk_get_int(thunk);
  return lsint_eq(intval, tintval) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

/**
 * Match a string value with a thunk
 * @param thunk The thunk
 * @param strval The string value
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_str(lsthunk_t *thunk, const lsstr_t *strval,
                                  lstenv_t *tenv) {
  (void)tenv;
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_STR)
    return LSMATCH_FAILURE;
  const lsstr_t *tstrval = lsthunk_get_str(thunk);
  return lsstrcmp(strval, tstrval) == 0 ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_pref(lsthunk_t *thunk, const lsref_t *ref,
                            lstenv_t *tenv) {
  const lsstr_t *refname = lsref_get_name(ref);
#ifdef DEBUG
  const lstref_t *tref = lstenv_get_self(tenv, refname);
  // tref must be NULL, otherwise it is a bug
  if (tref != NULL)
    assert(0);
#endif
  lstenv_put(
      tenv, refname,
      lstref_new_thunk(ref, thunk)); // TODO: should change to lstenv_put_thunk?
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pat(lsthunk_t *thunk, const lspat_t *pat,
                           lstenv_t *tenv) {
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE:
    return lsthunk_match_palge(thunk, lspat_get_alge(pat), tenv);
  case LSPTYPE_AS:
    return lsthunk_match_pas(thunk, lspat_get_as(pat), tenv);
  case LSPTYPE_INT:
    return lsthunk_match_int(thunk, lspat_get_int(pat), tenv);
  case LSPTYPE_STR:
    return lsthunk_match_str(thunk, lspat_get_str(pat), tenv);
  case LSPTYPE_REF:
    return lsthunk_match_pref(thunk, lspat_get_ref(pat), tenv);
  }
  return LSMATCH_FAILURE;
}

lsthunk_t *lsthunk_eval(lsthunk_t *thunk) {
  assert(thunk != NULL);
  if (thunk->lt_whnf != NULL)
    return thunk->lt_whnf;

  switch (thunk->lt_type) {
  case LSTTYPE_APPL:
    thunk->lt_whnf = lstappl_eval(thunk->lt_appl);
    break;
  case LSTTYPE_REF:
    thunk->lt_whnf = lstref_eval(thunk->lt_ref);
    break;
  case LSTTYPE_CHOICE:
    thunk->lt_whnf = lstchoice_eval(thunk->lt_choice);
    break;
  default:
    // it already is in WHNF
    thunk->lt_whnf = thunk;
    break;
  }
  return thunk->lt_whnf;
}

static lsthunk_t *lsthunk_apply_alge(lsthunk_t *thunk, const lstlist_t *args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_ALGE);
  assert(args != NULL);
  return lstalge_apply(lsthunk_get_alge(thunk), args);
}

static lsthunk_t *lsthunk_apply_appl(lsthunk_t *thunk, const lstlist_t *args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_APPL);
  assert(args != NULL);
  return lstappl_apply(lsthunk_get_appl(thunk), args);
}

static lsthunk_t *lsthunk_apply_lambda(lsthunk_t *thunk,
                                       const lstlist_t *args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  assert(args != NULL);
  return lstlambda_apply(lsthunk_get_lambda(thunk), args);
}

static lsthunk_t *lsthunk_apply_ref(lsthunk_t *thunk, const lstlist_t *args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_REF);
  assert(args != NULL);
  return lstref_apply(lsthunk_get_ref(thunk), args);
}

static lsthunk_t *lsthunk_apply_choice(lsthunk_t *thunk,
                                       const lstlist_t *args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  assert(args != NULL);
  return lstchoice_apply(lsthunk_get_choice(thunk), args);
}

lsthunk_t *lsthunk_apply(lsthunk_t *func, const lstlist_t *args) {
  assert(func != NULL);
  if (lstlist_count(args) == 0)
    return func;
  switch (lsthunk_get_type(func)) {
  case LSTTYPE_ALGE:
    return lsthunk_apply_alge(func, args);
  case LSTTYPE_APPL:
    return lsthunk_apply_appl(func, args);
  case LSTTYPE_LAMBDA:
    return lsthunk_apply_lambda(func, args);
  case LSTTYPE_REF:
    return lsthunk_apply_ref(func, args);
  case LSTTYPE_CHOICE:
    return lsthunk_apply_choice(func, args);
  case LSTTYPE_INT:
    lsprintf(stderr, 0, "F: cannot apply for integer\n");
    return NULL;
  case LSTTYPE_STR:
    lsprintf(stderr, 0, "F: cannot apply for string\n");
    return NULL;
  }
}
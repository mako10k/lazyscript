#include "thunk/thunk.h"
#include "common/io.h"
#include "common/loc.h"
#include "common/malloc.h"
#include "expr/eclosure.h"
#include "lstypes.h"
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

struct lstref_target_origin {
  lstrtype_t lrto_type;
  union {
    lstbind_t lrto_bind;
    lstlambda_t lrto_lambda;
    lsthunk_t *lrto_builtin;
  };
};

struct lstref_target {
  lstref_target_origin_t *lrt_origin;
  lstpat_t *lrt_pat;
};

struct lstref {
  const lsref_t *ltr_ref;
  lstref_target_t *ltr_target;
};

struct lstbuiltin {
  const lsstr_t *lti_name;
  lssize_t lti_arity;
  lstbuiltin_func_t lti_func;
  void *lti_data;
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
    const lstbuiltin_t *lt_builtin;
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
  lstref_target_origin_t *origins[ebindc];
  for (lssize_t i = 0; i < ebindc; i++) {
    const lspat_t *lhs = lsbind_get_lhs(ebinds[i]);
    origins[i] = lsmalloc(sizeof(lstref_target_origin_t));
    origins[i]->lrto_type = LSTRTYPE_BIND;
    origins[i]->lrto_bind.ltb_lhs = lstpat_new_pat(lhs, tenv, origins[i]);
    if (origins[i]->lrto_bind.ltb_lhs == NULL)
      return NULL;
  }
  for (lssize_t i = 0; i < ebindc; i++) {
    const lsexpr_t *rhs = lsbind_get_rhs(ebinds[i]);
    origins[i]->lrto_bind.ltb_rhs = lsthunk_new_expr(rhs, tenv);
    if (origins[i]->lrto_bind.ltb_rhs == NULL)
      return NULL;
  }
  return lsthunk_new_expr(lseclosure_get_expr(eclosure), tenv);
}

lsthunk_t *lsthunk_new_ref(const lsref_t *ref, lstenv_t *tenv) {
  lstref_target_t *target = lstenv_get(tenv, lsref_get_name(ref));
  lsthunk_t *thunk = lsmalloc(lssizeof(lsthunk_t, lt_ref));
  thunk->lt_type = LSTTYPE_REF;
  thunk->lt_whnf = NULL;
  thunk->lt_ref.ltr_ref = ref;
  thunk->lt_ref.ltr_target = target;
  if (target == NULL) {
    lsprintf(stderr, 0, "E: ");
    lsloc_print(stderr, lsref_get_loc(ref));
    lsprintf(stderr, 0, "undefined reference: ");
    lsref_print(stderr, LSPREC_LOWEST, 0, ref);
    lsprintf(stderr, 0, "\n");
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
  lstref_target_origin_t *origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type = LSTRTYPE_LAMBDA;
  origin->lrto_lambda.ltl_param = lstpat_new_pat(pparam, tenv, origin);
  if (origin->lrto_lambda.ltl_param == NULL)
    return NULL;
  origin->lrto_lambda.ltl_body = lsthunk_new_expr(ebody, tenv);
  if (origin->lrto_lambda.ltl_body == NULL)
    return NULL;
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_LAMBDA;
  thunk->lt_whnf = thunk;
  thunk->lt_lambda.ltl_param = origin->lrto_lambda.ltl_param;
  thunk->lt_lambda.ltl_body = origin->lrto_lambda.ltl_body;
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

lsmres_t lsthunk_match_alge(lsthunk_t *thunk, lstpat_t *tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_ALGE);
  lsthunk_t *thunk_whnf = lsthunk_eval0(thunk);
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
  if (ttype != LSTTYPE_ALGE)
    return LSMATCH_FAILURE; // TODO: match as list or string
  const lsstr_t *tconstr = lsthunk_get_constr(thunk_whnf);
  const lsstr_t *pconstr = lstpat_get_constr(tpat);
  if (lsstrcmp(pconstr, tconstr) != 0)
    return LSMATCH_FAILURE;
  lssize_t pargc = lstpat_get_argc(tpat);
  lssize_t targc = lsthunk_get_argc(thunk_whnf);
  if (pargc != targc)
    return LSMATCH_FAILURE;
  lstpat_t *const *pargs = lstpat_get_args(tpat);
  lsthunk_t *const *targs = lsthunk_get_args(thunk_whnf);
  for (lssize_t i = 0; i < pargc; i++)
    if (lsthunk_match_pat(targs[i], pargs[i]) < 0)
      return LSMATCH_FAILURE;
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pas(lsthunk_t *thunk, lstpat_t *tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_AS);
  lstpat_t *tpref = lstpat_get_ref(tpat);
  lstpat_t *tpaspattern = lstpat_get_aspattern(tpat);
  lsmres_t mres = lsthunk_match_pat(thunk, tpaspattern);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  mres = lsthunk_match_ref(thunk, tpref);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return LSMATCH_SUCCESS;
}

/**
 * Match an integer value with a thunk
 * @param thunk The thunk
 * @param tpat The integer value
 * @return The result
 */
static lsmres_t lsthunk_match_int(lsthunk_t *thunk, lstpat_t *tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_INT);
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_INT)
    return LSMATCH_FAILURE;
  const lsint_t *pintval = lstpat_get_int(tpat);
  const lsint_t *tintval = lsthunk_get_int(thunk);
  return lsint_eq(pintval, tintval) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

/**
 * Match a string value with a thunk
 * @param thunk The thunk
 * @param tpat The string value
 * @return The result
 */
static lsmres_t lsthunk_match_str(lsthunk_t *thunk, const lstpat_t *tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_STR);
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_STR)
    return LSMATCH_FAILURE; // TODO: match as list
  const lsstr_t *pstrval = lstpat_get_str(tpat);
  const lsstr_t *tstrval = lsthunk_get_str(thunk);
  return lsstrcmp(pstrval, tstrval) == 0 ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_ref(lsthunk_t *thunk, lstpat_t *tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_REF);
  lstpat_set_refbound(tpat, thunk);
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pat(lsthunk_t *thunk, lstpat_t *tpat) {
  switch (lstpat_get_type(tpat)) {
  case LSPTYPE_ALGE:
    return lsthunk_match_alge(thunk, tpat);
  case LSPTYPE_AS:
    return lsthunk_match_pas(thunk, tpat);
  case LSPTYPE_INT:
    return lsthunk_match_int(thunk, tpat);
  case LSPTYPE_STR:
    return lsthunk_match_str(thunk, tpat);
  case LSPTYPE_REF:
    return lsthunk_match_ref(thunk, tpat);
  }
  return LSMATCH_FAILURE;
}

static lsthunk_t *lsthunk_eval_alge(lsthunk_t *thunk, lssize_t argc,
                                    lsthunk_t *const *args) {
  // eval (C a b ...) x y ... = C a b ... x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_ALGE);
  if (argc == 0)
    return thunk; // it already is in WHNF
  lssize_t targc = lsthunk_get_argc(thunk);
  lsthunk_t *thunk_new = lsmalloc(lssizeof(lsthunk_t, lt_alge) +
                                  (targc + targc) * sizeof(lsthunk_t *));
  thunk_new->lt_type = LSTTYPE_ALGE;
  thunk_new->lt_whnf = thunk;
  thunk_new->lt_alge.lta_constr = thunk->lt_alge.lta_constr;
  thunk_new->lt_alge.lta_argc = targc + argc;
  for (lssize_t i = 0; i < targc; i++)
    thunk_new->lt_alge.lta_args[i] = thunk->lt_alge.lta_args[i];
  for (lssize_t i = 0; i < argc; i++)
    thunk_new->lt_alge.lta_args[targc + i] = args[i];
  return thunk_new;
}

static lsthunk_t *lsthunk_eval_appl(lsthunk_t *thunk, lssize_t argc,
                                    lsthunk_t *const *args) {
  // eval (f a b ...) x y ... => eval f a b ... x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_APPL);
  if (argc == 0)
    return lsthunk_eval0(thunk);
  lsthunk_t *func1 = thunk->lt_appl.lta_func;
  lssize_t argc1 = thunk->lt_appl.lta_argc + argc;
  lsthunk_t *const *args1 = (lsthunk_t *const *)lsa_concata(
      thunk->lt_appl.lta_argc, (const void *const *)thunk->lt_appl.lta_args,
      argc, (const void *const *)args);
  return lsthunk_eval(func1, argc1, args1);
}

static lsthunk_t *lsthunk_eval_lambda(lsthunk_t *thunk, lssize_t argc,
                                      lsthunk_t *const *args) {
  // eval (\param -> body) x y ... = eval (body[param := x]) y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  assert(argc > 0);
  assert(args != NULL);
  lstpat_t *param = lsthunk_get_param(thunk);
  lsthunk_t *body = lsthunk_get_body(thunk);
  lsthunk_t *arg;
  lsthunk_t *const *args1 = (lsthunk_t *const *)lsa_shift(
      argc, (const void *const *)args, (const void **)&arg);
  // Bind directly on the lambda's original parameter pattern so that
  // references inside body (which point to the same pattern objects via env)
  // observe the binding. We'll clear bindings after evaluation to keep
  // the lambda reusable.
  body = lsthunk_clone(body);
  lsmres_t mres = lsthunk_match_pat(arg, param);
  if (mres != LSMATCH_SUCCESS)
    return NULL;
  lsthunk_t *ret = lsthunk_eval(body, argc - 1, args1);
  // Clear parameter bindings for reuse
  lstpat_clear_binds(param);
  return ret;
}

static lsthunk_t *lsthunk_eval_builtin(lsthunk_t *thunk, lssize_t argc,
                                       lsthunk_t *const *args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_BUILTIN);
  if (argc == 0)
    return thunk;
  lssize_t arity = thunk->lt_builtin->lti_arity;
  lstbuiltin_func_t func = thunk->lt_builtin->lti_func;
  void *data = thunk->lt_builtin->lti_data;
  if (argc < arity) {
    lsthunk_t *ret =
        lsmalloc(lssizeof(lsthunk_t, lt_appl) + argc * sizeof(lsthunk_t *));
    ret->lt_type = LSTTYPE_APPL;
    ret->lt_whnf = ret;
    ret->lt_appl.lta_func = thunk;
    ret->lt_appl.lta_argc = argc;
    for (lssize_t i = 0; i < argc; i++)
      ret->lt_appl.lta_args[i] = args[i];
    return ret;
  }
  lsthunk_t *ret = func(arity, args, data);
  if (ret == NULL)
    return NULL;
  return lsthunk_eval(ret, argc - arity, args + arity);
}

static lsthunk_t *lsthunk_eval_ref(lsthunk_t *thunk, lssize_t argc,
                                   lsthunk_t *const *args) {
  // eval ~r x y ... = eval (eval ~r) x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_REF);
  assert(argc == 0 || args != NULL);
  lstref_target_t *target = thunk->lt_ref.ltr_target;
  assert(target != NULL);
  lstref_target_origin_t *origin = target->lrt_origin;
  assert(origin != NULL);
  lstpat_t *pat_ref = target->lrt_pat;
  assert(pat_ref != NULL);
  lsthunk_t *refbound = lstpat_get_refbound(pat_ref);
  if (refbound != NULL)
    return lsthunk_eval(refbound, argc, args);
  switch (origin->lrto_type) {
  case LSTRTYPE_BIND: {
    lsmres_t mres =
        lsthunk_match_pat(origin->lrto_bind.ltb_rhs, origin->lrto_bind.ltb_lhs);
    if (mres != LSMATCH_SUCCESS)
      return NULL;
    refbound = lstpat_get_refbound(pat_ref);
    assert(refbound != NULL);
    return lsthunk_eval(refbound, argc, args);
  }
  case LSTRTYPE_LAMBDA: {
    // Parameter refs should have been bound during lambda application.
    refbound = lstpat_get_refbound(pat_ref);
    if (refbound == NULL) {
      lsprintf(stderr, 0, "E: unbound lambda parameter reference\n");
      return NULL;
    }
    return lsthunk_eval(refbound, argc, args);
  }
  case LSTRTYPE_BUILTIN:
    return lsthunk_eval_builtin(origin->lrto_builtin, argc, args);
  }
}

static lsthunk_t *lsthunk_eval_choice(lsthunk_t *thunk, lssize_t argc,
                                      lsthunk_t *const *args) {
  // eval (l | r) x y ... = eval l x y ... | <differed> eval r x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  assert(argc == 0 || args != NULL);
  lsthunk_t *left = lsthunk_eval(thunk->lt_choice.ltc_left, argc, args);
  if (left == NULL)
    return lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
  // TODO: if left is fixed type, then we can skip right???
  lsthunk_t *right =
      lsmalloc(lssizeof(lsthunk_t, lt_appl) + (argc + 1) * sizeof(lsthunk_t *));
  right->lt_type = LSTTYPE_APPL;
  right->lt_whnf = NULL;
  right->lt_appl.lta_func = thunk->lt_choice.ltc_right;
  right->lt_appl.lta_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    right->lt_appl.lta_args[i] = args[i];
  lsthunk_t *choice = lsmalloc(sizeof(lsthunk_t));
  choice->lt_type = LSTTYPE_CHOICE;
  choice->lt_whnf = choice;
  choice->lt_choice.ltc_left = left;
  choice->lt_choice.ltc_right = right;
  return choice;
}

lsthunk_t *lsthunk_eval(lsthunk_t *func, lssize_t argc,
                        lsthunk_t *const *args) {
  assert(func != NULL);
  if (argc == 0)
    return lsthunk_eval0(func);
  switch (lsthunk_get_type(func)) {
  case LSTTYPE_ALGE:
    return lsthunk_eval_alge(func, argc, args);
  case LSTTYPE_APPL:
    return lsthunk_eval_appl(func, argc, args);
  case LSTTYPE_LAMBDA:
    return lsthunk_eval_lambda(func, argc, args);
  case LSTTYPE_REF:
    return lsthunk_eval_ref(func, argc, args);
  case LSTTYPE_CHOICE:
    return lsthunk_eval_choice(func, argc, args);
  case LSTTYPE_INT:
    lsprintf(stderr, 0, "F: cannot apply for integer\n");
    return NULL;
  case LSTTYPE_STR:
    lsprintf(stderr, 0, "F: cannot apply for string\n");
    return NULL;
  case LSTTYPE_BUILTIN:
    return lsthunk_eval_builtin(func, argc, args);
  }
}

lsthunk_t *lsthunk_eval0(lsthunk_t *thunk) {
  assert(thunk != NULL);
  if (thunk->lt_whnf != NULL)
    return thunk->lt_whnf;
  switch (thunk->lt_type) {
  case LSTTYPE_APPL:
    thunk->lt_whnf =
        lsthunk_eval(thunk->lt_appl.lta_func, thunk->lt_appl.lta_argc,
                     thunk->lt_appl.lta_args);
    break;
  case LSTTYPE_REF:
    thunk->lt_whnf = lsthunk_eval_ref(thunk, 0, NULL);
    break;
  case LSTTYPE_CHOICE:
    thunk->lt_whnf = lsthunk_eval_choice(thunk, 0, NULL);
    break;
  case LSTTYPE_BUILTIN:
    thunk->lt_whnf = lsthunk_eval_builtin(thunk, 0, NULL);
    break;
  default:
    // it already is in WHNF
    thunk->lt_whnf = thunk;
    break;
  }
  return thunk->lt_whnf;
}

lstref_target_t *lstref_target_new(lstref_target_origin_t *origin,
                                   lstpat_t *tpat) {
  lstref_target_t *target = lsmalloc(sizeof(lstref_target_t));
  target->lrt_origin = origin;
  target->lrt_pat = tpat;
  return target;
}

lsthunk_t *lsthunk_new_builtin(const lsstr_t *name, lssize_t arity,
                               lstbuiltin_func_t func, void *data) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_BUILTIN;
  thunk->lt_whnf = thunk;
  lstbuiltin_t *builtin = lsmalloc(sizeof(lstbuiltin_t));
  builtin->lti_name = name;
  builtin->lti_arity = arity;
  builtin->lti_func = func;
  builtin->lti_data = data;
  thunk->lt_builtin = builtin;
  return thunk;
}

lstref_target_origin_t *lstref_target_origin_new_builtin(const lsstr_t *name,
                                                         lssize_t arity,
                                                         lstbuiltin_func_t func,
                                                         void *data) {
  lstref_target_origin_t *origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type = LSTRTYPE_BUILTIN;
  origin->lrto_builtin = lsthunk_new_builtin(name, arity, func, data);
  return origin;
}

lsthunk_t *lsprog_eval(const lsprog_t *prog, lstenv_t *tenv) {
  lsthunk_t *thunk = lsthunk_new_expr(lsprog_get_expr(prog), tenv);
  if (thunk == NULL)
    return NULL;
  return lsthunk_eval0(thunk);
}

typedef enum lsprint_mode { LSPM_SHARROW, LSPM_ASIS, LSPM_DEEP } lsprint_mode_t;

typedef struct lsthunk_colle {
  // thunk entry
  lsthunk_t *ltc_thunk;
  // thunk id
  lssize_t ltc_id;
  // duplicated count
  lssize_t ltc_count;
  // minimal level
  lssize_t ltc_level;
  // next thunk entry
  struct lsthunk_colle *ltc_next;
} lsthunk_colle_t;

lsthunk_colle_t **lsthunk_colle_find(lsthunk_colle_t **pcolle,
                                     const lsthunk_t *thunk) {
  assert(pcolle != NULL);
  while (*pcolle != NULL) {
    if ((*pcolle)->ltc_thunk == thunk)
      break;
    pcolle = &(*pcolle)->ltc_next;
  }
  return pcolle;
}

static lsthunk_colle_t *lsthunk_colle_new(lsthunk_t *thunk,
                                          lsthunk_colle_t *head, lssize_t *pid,
                                          lssize_t level, lsprint_mode_t mode) {
  switch (mode) {
  case LSPM_SHARROW:
    break;
  case LSPM_ASIS:
    thunk = thunk->lt_whnf != NULL ? thunk->lt_whnf : thunk;
    break;
  case LSPM_DEEP:
    thunk = lsthunk_eval0(thunk);
    break;
  }
  lsthunk_colle_t **pcolle = lsthunk_colle_find(&head, thunk);
  if (*pcolle != NULL) {
    (*pcolle)->ltc_id = (*pid)++;
    (*pcolle)->ltc_count++;
    if ((*pcolle)->ltc_level > level)
      (*pcolle)->ltc_level = level;
    return head;
  }
  *pcolle = lsmalloc(sizeof(lsthunk_colle_t));
  (*pcolle)->ltc_thunk = thunk;
  (*pcolle)->ltc_id = 0;
  (*pcolle)->ltc_count = 1;
  (*pcolle)->ltc_level = level;
  (*pcolle)->ltc_next = NULL;
  switch (thunk->lt_type) {
  case LSTTYPE_ALGE:
    for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++)
      head = lsthunk_colle_new(thunk->lt_alge.lta_args[i], head, pid, level + 1,
                               mode);
    break;
  case LSTTYPE_APPL:
    head =
        lsthunk_colle_new(thunk->lt_appl.lta_func, head, pid, level + 1, mode);
    for (lssize_t i = 0; i < thunk->lt_appl.lta_argc; i++)
      head = lsthunk_colle_new(thunk->lt_appl.lta_args[i], head, pid, level + 1,
                               mode);
    break;
  case LSTTYPE_CHOICE:
    head = lsthunk_colle_new(thunk->lt_choice.ltc_left, head, pid, level + 1,
                             mode);
    head = lsthunk_colle_new(thunk->lt_choice.ltc_right, head, pid, level + 1,
                             mode);
    break;
  case LSTTYPE_LAMBDA:
    head = lsthunk_colle_new(thunk->lt_lambda.ltl_body, head, pid, level + 1,
                             mode);
    break;
  case LSTTYPE_REF:
  case LSTTYPE_INT:
  case LSTTYPE_STR:
  case LSTTYPE_BUILTIN:
    break;
  }
  return head;
}

static void lsthunk_print_internal(FILE *fp, lsprec_t prec, int indent,
                                   lsthunk_t *thunk, lssize_t level,
                                   lsthunk_colle_t *colle, lsprint_mode_t mode,
                                   int force_print) {
  switch (mode) {
  case LSPM_SHARROW:
    break;
  case LSPM_ASIS:
    thunk = thunk->lt_whnf != NULL ? thunk->lt_whnf : thunk;
    break;
  case LSPM_DEEP:
    thunk = lsthunk_eval0(thunk);
    break;
  }
  int has_dup = 0;
  lsthunk_colle_t *colle_found = NULL;
  for (lsthunk_colle_t *c = colle; c != NULL; c = c->ltc_next) {
    if (c->ltc_count > 1 && c->ltc_level == level) {
      has_dup = 1;
      if (colle_found != NULL)
        break;
    }
    if (c->ltc_thunk == thunk) {
      colle_found = c;
      if (has_dup)
        break;
    }
  }
  assert(colle_found != NULL);

  if (has_dup)
    lsprintf(fp, ++indent, "(\n");

  if (!force_print && colle_found->ltc_count > 1)
    lsprintf(fp, 0, "~__ref%u", colle_found->ltc_id);
  else
    switch (thunk->lt_type) {
    case LSTTYPE_ALGE:
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr("[]")) == 0) {
        lsprintf(fp, 0, "[");
        for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
          if (i > 0)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent,
                                 thunk->lt_alge.lta_args[i], level + 1, colle,
                                 mode, 0);
        }
        lsprintf(fp, 0, "]");
        return;
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(",")) == 0) {
        lsprintf(fp, 0, "(");
        for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
          if (i > 0)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent,
                                 thunk->lt_alge.lta_args[i], level + 1, colle,
                                 mode, 0);
        }
        lsprintf(fp, 0, ")");
        return;
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
          thunk->lt_alge.lta_argc == 2) {
        if (prec > LSPREC_CONS)
          lsprintf(fp, 0, "(");
        lsthunk_print_internal(fp, LSPREC_CONS + 1, indent,
                               thunk->lt_alge.lta_args[0], level + 1, colle,
                               mode, 0);
        lsprintf(fp, 0, " : ");
        lsthunk_print_internal(fp, LSPREC_CONS, indent,
                               thunk->lt_alge.lta_args[1], level + 1, colle,
                               mode, 0);
        if (prec > LSPREC_CONS)
          lsprintf(fp, 0, ")");
        return;
      }
      if (thunk->lt_alge.lta_argc == 0) {
        lsstr_print_bare(fp, prec, indent, thunk->lt_alge.lta_constr);
        return;
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, indent, "(");
      lsstr_print_bare(fp, prec, indent, thunk->lt_alge.lta_constr);
      for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
        lsprintf(fp, indent, " ");
        lsthunk_print_internal(fp, LSPREC_APPL + 1, indent,
                               thunk->lt_alge.lta_args[i], level + 1, colle,
                               mode, 0);
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, 0, ")");
      break;
    case LSTTYPE_APPL:
      if (thunk->lt_appl.lta_argc == 0) {
        lsthunk_print_internal(fp, prec, indent, thunk->lt_appl.lta_func,
                               level + 1, colle, mode, 0);
        return;
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, 0, "(");
      lsthunk_print(fp, LSPREC_APPL + 1, indent, thunk->lt_appl.lta_func);
      for (lssize_t i = 0; i < thunk->lt_appl.lta_argc; i++) {
        lsprintf(fp, indent, " ");
        lsthunk_print_internal(fp, LSPREC_APPL + 1, indent,
                               thunk->lt_appl.lta_args[i], level + 1, colle,
                               mode, 0);
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, indent, ")");
      break;
    case LSTTYPE_CHOICE:
      if (prec > LSPREC_CHOICE)
        lsprintf(fp, 0, "(");
      lsthunk_print_internal(fp, LSPREC_CHOICE + 1, indent,
                             thunk->lt_choice.ltc_left, level + 1, colle, mode,
                             0);
      lsprintf(fp, 0, " | ");
      lsthunk_print_internal(fp, LSPREC_CHOICE, indent,
                             thunk->lt_choice.ltc_right, level + 1, colle, mode,
                             0);
      if (prec > LSPREC_CHOICE)
        lsprintf(fp, 0, ")");
      break;
    case LSTTYPE_LAMBDA:
      if (prec > LSPREC_LAMBDA)
        lsprintf(fp, indent, "(");
      lsprintf(fp, indent, "\\");
      lstpat_print(fp, LSPREC_APPL + 1, indent, thunk->lt_lambda.ltl_param);
      lsprintf(fp, indent, " -> ");
      lsthunk_print_internal(fp, LSPREC_LAMBDA, indent,
                             thunk->lt_lambda.ltl_body, level + 1, colle, mode,
                             0);
      if (prec > LSPREC_LAMBDA)
        lsprintf(fp, indent, ")");
      break;
    case LSTTYPE_REF:
      lsref_print(fp, prec, indent, thunk->lt_ref.ltr_ref);
      break;
    case LSTTYPE_INT:
      lsint_print(fp, prec, indent, thunk->lt_int);
      break;
    case LSTTYPE_STR:
      lsstr_print(fp, prec, indent, thunk->lt_str);
      break;
    case LSTTYPE_BUILTIN:
      lsprintf(fp, indent, "~%s{-builtin/%d-}",
               lsstr_get_buf(thunk->lt_builtin->lti_name),
               thunk->lt_builtin->lti_arity);
      break;
    }
  if (has_dup) {
    for (lsthunk_colle_t *c = colle; c != NULL; c = c->ltc_next) {
      if (c->ltc_level == level && c->ltc_count > 1) {
        lsprintf(fp, indent, ";\n~__ref%u = ", c->ltc_id);
        lsthunk_print_internal(fp, LSPREC_LOWEST, indent, c->ltc_thunk,
                               level + 1, colle, mode, 1);
      }
    }
    lsprintf(fp, --indent, "\n)");
  }
}

void lsthunk_print(FILE *fp, lsprec_t prec, int indent, lsthunk_t *thunk) {
  lssize_t id = 0;
  lsthunk_colle_t *colle = lsthunk_colle_new(thunk, NULL, &id, 0, 0);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_SHARROW, 0);
}

void lsthunk_dprint(FILE *fp, lsprec_t prec, int indent, lsthunk_t *thunk) {
  lssize_t id = 0;
  lsthunk_colle_t *colle = lsthunk_colle_new(thunk, NULL, &id, 0, 1);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_ASIS, 0);
}

lsthunk_t *lsthunk_clone(lsthunk_t *thunk) {
  // Thunks are treated as immutable graph nodes managed by GC.
  // Returning the same pointer is sufficient for now.
  return thunk;
}
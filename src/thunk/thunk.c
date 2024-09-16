#include "thunk/thunk.h"
#include "common/io.h"
#include "common/loc.h"
#include "common/malloc.h"
#include "common/str.h"
#include "expr/eclosure.h"
#include "lsassert.h"
#include "lstypes.h"
#include "thunk/eenv.h"
#include "thunk/tbind.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"
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

struct lstbindref {
  lstbind_t *ltbr_bind;
  const lsref_t *ltbr_ref;
  lsthunk_t *ltbr_bound;
};

struct lstparamref {
  lsthunk_t *ltpr_lambda;
  const lsref_t *ltpr_ref;
  lsthunk_t *ltpr_bound;
};

struct lstthunkref {
  const lsref_t *lttr_ref;
  lsthunk_t *lttr_bound;
};

struct lstbuiltin {
  const lsstr_t *lti_name;
  lssize_t lti_arity;
  lstbuiltin_func_t lti_func;
  void *lti_data;
};

struct lstbeta {
  const lstenv_t *lste_env;
  const lsthunk_t *lste_thunk;
};

struct lsthunk {
  lsttype_t lt_type;
  lsthunk_t *lt_whnf;
  union {
    lstbeta_t lt_beta;
    lstalge_t lt_alge;
    lstappl_t lt_appl;
    lstbindref_t lt_bindref;
    lstbuiltin_t lt_builtin;
    lstchoice_t lt_choice;
    const lsint_t *lt_int;
    lstlambda_t lt_lambda;
    lstparamref_t lt_paramref;
    const lsstr_t *lt_str;
    lstthunkref_t lt_thunkref;
  };
};

// *************************************
// Simple constructors
// *************************************

lsthunk_t *lsthunk_new_beta(const lstenv_t *env, const lsthunk_t *thunk) {
  lsthunk_t *t = lsmalloc(lssizeof(lsthunk_t, lt_beta));
  t->lt_type = LSTTYPE_BETA;
  t->lt_whnf = NULL;
  t->lt_beta.lste_env = env;
  t->lt_beta.lste_thunk = thunk;
  return t;
}

lsthunk_t *lsthunk_new_alge(const lsstr_t *constr, lssize_t argc,
                            lsthunk_t *const *args) {
  lsassert_args(argc, args);
  lsthunk_t *thunk =
      lsmalloc(lssizeof(lsthunk_t, lt_alge) + argc * sizeof(lsthunk_t *));
  thunk->lt_type = LSTTYPE_ALGE;
  thunk->lt_whnf = thunk;
  thunk->lt_alge.lta_constr = constr;
  thunk->lt_alge.lta_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    thunk->lt_alge.lta_args[i] = args[i];
  return thunk;
}

lsthunk_t *lsthunk_new_appl(lsthunk_t *func, lssize_t argc,
                            lsthunk_t *const *args) {
  lsassert_args(argc, args);
  lsthunk_t *thunk =
      lsmalloc(lssizeof(lsthunk_t, lt_appl) + argc * sizeof(lsthunk_t *));
  thunk->lt_type = LSTTYPE_APPL;
  thunk->lt_whnf = NULL;
  thunk->lt_appl.lta_func = func;
  thunk->lt_appl.lta_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    thunk->lt_appl.lta_args[i] = args[i];
  return thunk;
}

lsthunk_t *lsthunk_new_bindref(lstbind_t *bind, const lsref_t *ref,
                               lsthunk_t *bound) {
  lsthunk_t *thunk = lsmalloc(lssizeof(lsthunk_t, lt_bindref));
  thunk->lt_type = LSTTYPE_BINDREF;
  thunk->lt_whnf = NULL;
  thunk->lt_bindref.ltbr_bind = bind;
  thunk->lt_bindref.ltbr_ref = ref;
  thunk->lt_bindref.ltbr_bound = bound;
  return thunk;
}

lsthunk_t *lsthunk_new_builtin(const lsstr_t *name, lssize_t arity,
                               lstbuiltin_func_t func, void *data) {
  lsthunk_t *thunk = lsmalloc(lssizeof(lsthunk_t, lt_builtin));
  thunk->lt_type = LSTTYPE_BUILTIN;
  thunk->lt_whnf = thunk;
  thunk->lt_builtin.lti_name = name;
  thunk->lt_builtin.lti_arity = arity;
  thunk->lt_builtin.lti_func = func;
  thunk->lt_builtin.lti_data = data;
  return thunk;
}

lsthunk_t *lsthunk_new_choice(lsthunk_t *left, lsthunk_t *right) {
  if (left == NULL)
    return right;
  if (right == NULL)
    return left;
  lsthunk_t *thunk = lsmalloc(lssizeof(lsthunk_t, lt_choice));
  thunk->lt_type = LSTTYPE_CHOICE;
  thunk->lt_whnf = NULL;
  thunk->lt_choice.ltc_left = left;
  thunk->lt_choice.ltc_right = right;
  return thunk;
}

lsthunk_t *lsthunk_new_int(const lsint_t *intval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_INT;
  thunk->lt_whnf = thunk;
  thunk->lt_int = intval;
  return thunk;
}

lsthunk_t *lsthunk_new_lambda(lstpat_t *param, lsthunk_t *body) {
  lsthunk_t *thunk = lsmalloc(lssizeof(lsthunk_t, lt_lambda));
  thunk->lt_type = LSTTYPE_LAMBDA;
  thunk->lt_whnf = thunk;
  thunk->lt_lambda.ltl_param = param;
  thunk->lt_lambda.ltl_body = body;
  return thunk;
}

lsthunk_t *lsthunk_new_paramref(lsthunk_t *lambda, const lsref_t *ref,
                                lsthunk_t *bound) {
  lsassert(lambda != NULL);
  lsassert(lambda->lt_type == LSTTYPE_LAMBDA);
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_PARAMREF;
  thunk->lt_whnf = NULL;
  thunk->lt_paramref.ltpr_lambda = lambda;
  thunk->lt_paramref.ltpr_ref = ref;
  thunk->lt_paramref.ltpr_bound = bound;
  return thunk;
}

lsthunk_t *lsthunk_new_str(const lsstr_t *strval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_STR;
  thunk->lt_whnf = thunk;
  thunk->lt_str = strval;
  return thunk;
}

lsthunk_t *lsthunk_new_thunkref(const lsref_t *ref, lsthunk_t *bound) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_THUNKREF;
  thunk->lt_whnf = NULL;
  thunk->lt_thunkref.lttr_ref = ref;
  thunk->lt_thunkref.lttr_bound = bound;
  return thunk;
}

// *************************************
// Constructors from expressions
// *************************************

lsthunk_t *lsthunk_new_ealge(const lsealge_t *const ealge,
                             lseenv_t *const eenv) {
  const lsstr_t *const econstr = lsealge_get_constr(ealge);
  const lssize_t eargc = lsealge_get_argc(ealge);
  const lsexpr_t *const *const eargs = lsealge_get_args(ealge);

  const lsstr_t *const tconstr = lsealge_get_constr(ealge);
  const lssize_t targc = eargc;
  lsthunk_t *targs[eargc];
  for (lssize_t i = 0; i < eargc; i++) {
    lsthunk_t *const targ = lsthunk_new_expr(eargs[i], eenv);
    if (targ == NULL)
      return NULL;
    targs[i] = targ;
  }
  lsthunk_t *const thunk = lsthunk_new_alge(tconstr, targc, targs);
  return thunk;
}

lsthunk_t *lsthunk_new_eappl(const lseappl_t *eappl, lseenv_t *eenv) {
  const lsexpr_t *const efunc = lseappl_get_func(eappl);
  const lssize_t eargc = lseappl_get_argc(eappl);
  const lsexpr_t *const *const eargs = lseappl_get_args(eappl);

  lsthunk_t *tfunc = lsthunk_new_expr(efunc, eenv);
  if (tfunc == NULL)
    return NULL;
  const lssize_t targc = eargc;
  lsthunk_t *targs[eargc];
  for (lssize_t i = 0; i < eargc; i++) {
    lsthunk_t *const targ = lsthunk_new_expr(eargs[i], eenv);
    if (targ == NULL)
      return NULL;
    targs[i] = targ;
  }
  lsthunk_t *const thunk = lsthunk_new_appl(tfunc, eargc, targs);
  return thunk;
}

lsthunk_t *lsthunk_new_echoice(const lsechoice_t *echoice, lseenv_t *eenv) {
  const lsexpr_t *eleft = lsechoice_get_left(echoice);
  lsthunk_t *tleft = lsthunk_new_expr(eleft, eenv);
  if (tleft == NULL)
    return NULL;
  const lsexpr_t *eright = lsechoice_get_right(echoice);
  lsthunk_t *tright = lsthunk_new_expr(eright, eenv);
  if (tright == NULL)
    return NULL;
  return lsthunk_new_choice(tleft, tright);
}

lsthunk_t *lsthunk_new_eclosure(const lseclosure_t *eclosure, lseenv_t *eenv) {
  lseenv_t *eenv_clo = lseenv_new(eenv);
  lssize_t ebindc = lseclosure_get_bindc(eclosure);
  const lsbind_t *const *ebinds = lseclosure_get_binds(eclosure);
  lstpat_t *lhses[ebindc];
  lseref_target_origin_t *origins[ebindc];
  for (lssize_t i = 0; i < ebindc; i++) {
    const lspat_t *lhs = lsbind_get_lhs(ebinds[i]);
    lstpat_t *lhs_new = lstpat_new_pat(lhs, eenv_clo, NULL);
    if (lhs_new == NULL)
      return NULL;
    lhses[i] = lhs_new;
  }
  for (lssize_t i = 0; i < ebindc; i++) {
    const lsexpr_t *rhs = lsbind_get_rhs(ebinds[i]);
    lsthunk_t *rhs_new = lsthunk_new_expr(rhs, eenv_clo);
    if (rhs_new == NULL)
      return NULL;
    lstbind_t *tbind = lstbind_new(lhses[i], rhs_new);
    origins[i] = lseref_target_origin_new_bind(tbind);
  }
  const lsexpr_t *expr = lseclosure_get_expr(eclosure);
  lsthunk_t *texpr = lsthunk_new_expr(expr, eenv_clo);
  return texpr;
}

lsthunk_t *lsthunk_new_elambda(const lselambda_t *elambda, lseenv_t *eenv) {
  const lspat_t *eparam = lselambda_get_param(elambda);
  const lsexpr_t *ebody = lselambda_get_body(elambda);

  lsthunk_t *tlambda = lsmalloc(lssizeof(lsthunk_t, lt_lambda));
  tlambda->lt_type = LSTTYPE_LAMBDA;
  tlambda->lt_whnf = tlambda;
  tlambda->lt_lambda.ltl_param = NULL;
  tlambda->lt_lambda.ltl_body = NULL;
  lseref_target_origin_t *origin = lseref_target_origin_new_lambda(tlambda);
  lseenv_t *eenv_lam = lseenv_new(eenv);
  lstpat_t *tparam = lstpat_new_pat(eparam, eenv_lam, origin);
  lsthunk_t *tbody = lsthunk_new_expr(ebody, eenv_lam);
  tlambda->lt_lambda.ltl_param = tparam;
  tlambda->lt_lambda.ltl_body = tbody;
  return tlambda;
}

lsthunk_t *lsthunk_new_eref(const lsref_t *ref, lseenv_t *eenv) {
  lseref_target_t *target = lseenv_get(eenv, lsref_get_name(ref));
  // target will be one of below:
  //     1. lambda parameter reference
  //     2. bind reference
  //     3. direct expression / pre-resolved bind reference
  //     4. direct expression / builtin function
  if (target == NULL) {
    lsprintf(stderr, 0, "E: ");
    lsloc_print(stderr, lsref_get_loc(ref));
    lsprintf(stderr, 0, "undefined reference: ");
    lsref_print(stderr, LSPREC_LOWEST, 0, ref);
    lsprintf(stderr, 0, "\n");
    lseenv_incr_nerrors(eenv);
    return NULL;
  }
  switch (lseref_target_get_type(target)) {
  case LSERTYPE_BIND: {
    lstbind_t *bind = lseref_target_get_bind(target);
    return lsthunk_new_bindref(bind, ref, NULL);
  }
  case LSERTYPE_LAMBDA: {
    lsthunk_t *lambda = lseref_target_get_lambda(target);
    return lsthunk_new_paramref(lambda, ref, NULL);
  }
  case LSERTYPE_EXPR: {
    lsthunk_t *thunk = lseref_target_get_thunk(target);
    return lsthunk_new_thunkref(ref, thunk);
  }
  }
  lsassert_notreach();
}

lsthunk_t *lsthunk_new_expr(const lsexpr_t *expr, lseenv_t *eenv) {
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_ALGE:
    return lsthunk_new_ealge(lsexpr_get_alge(expr), eenv);
  case LSETYPE_APPL:
    return lsthunk_new_eappl(lsexpr_get_appl(expr), eenv);
  case LSETYPE_CHOICE:
    return lsthunk_new_echoice(lsexpr_get_choice(expr), eenv);
  case LSETYPE_CLOSURE:
    return lsthunk_new_eclosure(lsexpr_get_closure(expr), eenv);
  case LSETYPE_LAMBDA:
    return lsthunk_new_elambda(lsexpr_get_lambda(expr), eenv);
  case LSETYPE_INT:
    return lsthunk_new_int(lsexpr_get_int(expr));
  case LSETYPE_REF:
    return lsthunk_new_eref(lsexpr_get_ref(expr), eenv);
  case LSETYPE_STR:
    return lsthunk_new_str(lsexpr_get_str(expr));
  }
  lsassert_notreach();
}

// *************************************
// Accessors
// *************************************

lsttype_t lsthunk_get_type(const lsthunk_t *thunk) {
  lsassert(thunk != NULL);
  return thunk->lt_type;
}

lsbool_t lsthunk_is_list(const lsthunk_t *thunk) {
  return thunk->lt_type == LSTTYPE_ALGE &&
         lsstrcmp(lsthunk_get_constr(thunk), lsconstr_list()) == 0;
}

lsbool_t lsthunk_is_cons(const lsthunk_t *thunk) {
  return thunk->lt_type == LSTTYPE_ALGE &&
         lsstrcmp(lsthunk_get_constr(thunk), lsconstr_cons()) == 0 &&
         lsthunk_get_argc(thunk) == 2;
}

lsbool_t lsthunk_is_nil(const lsthunk_t *thunk) {
  return lsthunk_is_list(thunk) && lsthunk_get_argc(thunk) == 0;
}

const lsthunk_t *lsthunk_get_nil(void) {
  static lsthunk_t *nil = NULL;
  if (nil == NULL) {
    nil = lsthunk_new_alge(lsconstr_list(), 0, NULL);
  }
  return nil;
}

lsthunk_t *lsthunk_new_list(lssize_t argc, lsthunk_t *const *args) {
  lsassert_args(argc, args);
  return lsthunk_new_alge(lsconstr_list(), argc, args);
}

lsthunk_t *lsthunk_new_cons(lsthunk_t *head, lsthunk_t *tail) {
  lsthunk_t *args[2] = {head, tail};
  return lsthunk_new_alge(lsconstr_cons(), 2, args);
}

const lsstr_t *lsthunk_get_constr(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge.lta_constr;
}

lsthunk_t *lsthunk_get_func(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_appl.lta_func;
}

lssize_t lsthunk_get_argc(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_argc
                                        : thunk->lt_appl.lta_argc;
}

lsthunk_t *const *lsthunk_get_args(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_args
                                        : thunk->lt_appl.lta_args;
}

const lsint_t *lsthunk_get_int(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}

const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}

lsthunk_t *lsthunk_get_left(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice.ltc_left;
}

lsthunk_t *lsthunk_get_right(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice.ltc_right;
}

lstpat_t *lsthunk_get_param(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_param;
}

lsthunk_t *lsthunk_get_body(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_body;
}

lstbind_t *lsthunk_get_bind(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_BINDREF);
  return thunk->lt_bindref.ltbr_bind;
}

lsthunk_t *lsthunk_get_lambda(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_PARAMREF);
  return thunk->lt_paramref.ltpr_lambda;
}

const lsref_t *lsthunk_get_ref(const lsthunk_t *thunk) {
  switch (thunk->lt_type) {
  case LSTTYPE_BINDREF:
    return thunk->lt_bindref.ltbr_ref;
  case LSTTYPE_PARAMREF:
    return thunk->lt_paramref.ltpr_ref;
  case LSTTYPE_THUNKREF:
    return thunk->lt_thunkref.lttr_ref;
  default:
    lsassert_notreach();
  }
}

lsthunk_t *lsthunk_get_bound(const lsthunk_t *thunk) {
  switch (thunk->lt_type) {
  case LSTTYPE_BINDREF:
    return thunk->lt_bindref.ltbr_bound;
  case LSTTYPE_PARAMREF:
    return thunk->lt_paramref.ltpr_bound;
  case LSTTYPE_THUNKREF:
    return thunk->lt_thunkref.lttr_bound;
  default:
    lsassert_notreach();
  }
}

static void lsthunk_set_bound(lsthunk_t *thunk, lsthunk_t *bound) {
  switch (thunk->lt_type) {
  case LSTTYPE_BINDREF:
    thunk->lt_bindref.ltbr_bound = bound;
    return;
  case LSTTYPE_PARAMREF:
    thunk->lt_paramref.ltpr_bound = bound;
    return;
  case LSTTYPE_THUNKREF:
    thunk->lt_thunkref.lttr_bound = bound;
    return;
  default:
    lsassert_notreach();
  }
}

const lsstr_t *lsthunk_get_builtin_name(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_BUILTIN);
  return thunk->lt_builtin.lti_name;
}

lssize_t lsthunk_get_builtin_arity(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_BUILTIN);
  return thunk->lt_builtin.lti_arity;
}

lstbuiltin_func_t lsthunk_get_builtin_func(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_BUILTIN);
  return thunk->lt_builtin.lti_func;
}

void *lsthunk_get_builtin_data(const lsthunk_t *thunk) {
  lsassert(thunk->lt_type == LSTTYPE_BUILTIN);
  return thunk->lt_builtin.lti_data;
}

// *************************************
// Evaluation
// *************************************

lsthunk_t *lsthunk_eval(lsthunk_t *thunk) {
  lsassert(thunk != NULL);
  lsthunk_t *thunk_whnf = thunk->lt_whnf;
  if (thunk_whnf != NULL)
    return thunk_whnf;
  switch (thunk->lt_type) {
  case LSTTYPE_ALGE:
    return lsthunk_eval_alge(thunk, 0, NULL); // MUST NOT HAPPEN
  case LSTTYPE_APPL:
    return lsthunk_eval_appl(thunk, 0, NULL);
  case LSTTYPE_BETA:
    return lsthunk_eval_beta(thunk, 0, NULL);
  case LSTTYPE_BINDREF:
    return lsthunk_eval_ref(thunk, 0, NULL);
  case LSTTYPE_BUILTIN:
    return lsthunk_eval_builtin(thunk, 0, NULL);
  case LSTTYPE_CHOICE:
    return lsthunk_eval_choice(thunk, 0, NULL);
  case LSTTYPE_INT:
    return thunk_whnf; // MUST NOT HAPPEN
  case LSTTYPE_LAMBDA:
    return thunk_whnf; // MUST NOT HAPPEN
  case LSTTYPE_PARAMREF:
    return lsthunk_eval_ref(thunk, 0, NULL);
  case LSTTYPE_STR:
    return thunk_whnf; // MUST NOT HAPPEN
  case LSTTYPE_THUNKREF:
    return lsthunk_eval_ref(thunk, 0, NULL);
  }
  lsassert_notreach(); // MUST NOT HAPPEN
}

lsthunk_t *lsthunk_eval_appl(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args) {
  lsassert_tappl(thunk);
  lsassert_args(argc, args);
  lsthunk_t *func = lsthunk_get_func(thunk);
  lsthunk_t *func_whnf = lsthunk_eval(func);
  if (func_whnf == NULL || argc == 0)
    return func_whnf;
  lsttype_t func_type = lsthunk_get_type(func_whnf);
  switch (func_type) {
  case LSTTYPE_ALGE:
    return lsthunk_eval_alge(func_whnf, argc, args);
  case LSTTYPE_APPL:
    lsassert_notreach(); // MUST NOT HAPPEN
  case LSTTYPE_BETA:
    return lsthunk_eval_beta(func_whnf, argc, args);
  case LSTTYPE_BINDREF:
    lsassert_notreach(); // MUST NOT HAPPEN
  case LSTTYPE_BUILTIN:
    return lsthunk_eval_builtin(func_whnf, argc, args);
  case LSTTYPE_CHOICE:
    lsassert_notreach(); // MUST NOT HAPPEN
  case LSTTYPE_INT:
    return NULL; // Cannot Applicable
  case LSTTYPE_LAMBDA:
    return lsthunk_eval_lambda(func_whnf, argc, args);
  case LSTTYPE_PARAMREF:
    lsassert_notreach(); // MUST NOT HAPPEN
  case LSTTYPE_STR:
    return NULL; // Cannot Applicable
  case LSTTYPE_THUNKREF:
    lsassert_notreach(); // MUST NOT HAPPEN
  }
  lsassert_notreach(); // MUST NOT HAPPEN
}

lsthunk_t *lsthunk_eval_alge(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args) {
  lsassert_talge(thunk);
  lsassert_args(argc, args);
  if (argc == 0)
    return thunk;
  lssize_t targc = lsthunk_get_argc(thunk);
  lsthunk_t *const *targs = lsthunk_get_args(thunk);
  if (targc == 0)
    return lsthunk_new_alge(lsthunk_get_constr(thunk), argc, args);
  lsthunk_t *new_args[targc + argc];
  for (lssize_t i = 0; i < targc; i++)
    new_args[i] = targs[i];
  for (lssize_t i = 0; i < argc; i++)
    new_args[targc + i] = args[i];
  return lsthunk_new_alge(lsthunk_get_constr(thunk), targc + argc, new_args);
}

// *************************************
// Runtime pattern matching
// *************************************

/**
 * Match a thunk with a pattern
 * @param thunk The thunk (must be an algebraic thunk)
 * @param tpat The pattern (must be an algebraic pattern)
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_talge_palge_simple(lsthunk_t *thunk,
                                                 const lstpat_t *tpat,
                                                 lstenv_t *tenv) {
  // TALGE <-> TPALGE
  lsassert_talge(thunk);
  lsassert_tpalge(tpat);
  const lsstr_t *pconstr = lstpat_get_constr(tpat);
  const lsstr_t *tconstr = lsthunk_get_constr(thunk);
  if (lsstrcmp(tconstr, pconstr) != 0)
    return LSMATCH_FAILURE;
  lssize_t targc = lsthunk_get_argc(thunk);
  lssize_t pargc = lstpat_get_argc(tpat);
  if (targc != pargc)
    return LSMATCH_FAILURE;
  lsthunk_t *const *const targs = lsthunk_get_args(thunk);
  lstpat_t *const *const pargs = lstpat_get_args(tpat);
  for (lssize_t i = 0; i < targc; i++) {
    lsmres_t mres = lsthunk_match_pat(targs[i], pargs[i], tenv);
    if (mres != LSMATCH_SUCCESS)
      return mres;
  }
  return LSMATCH_SUCCESS;
}

/**
 * Match a thunk with a pattern
 * @param thunk The thunk (must be a string thunk)
 * @param tpat The pattern (must be a cons pattern)
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_tstr_pcons(lsthunk_t *thunk, const lstpat_t *tpat,
                                         lstenv_t *tenv) {
  // TSTR <-> TPCONS
  lsassert_tstr(thunk);
  lsassert_tpcons(tpat);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
  if (ttype != LSTTYPE_STR)
    return LSMATCH_FAILURE;
  const lsstr_t *tstrval = lsthunk_get_str(thunk_whnf);
  lssize_t tstrlen = lsstrlen(tstrval);
  if (tstrlen == 0)
    return LSMATCH_FAILURE;
  const lsint_t *int_thead = lsint_new(*lsstr_get_buf(tstrval) & 255);
  const lsstr_t *str_ttail = lsstr_sub(tstrval, 1, tstrlen - 1);
  lsthunk_t *thead = lsthunk_new_int(int_thead);
  lsthunk_t *ttail = lsthunk_new_str(str_ttail);
  lstpat_t *const *pargs = lstpat_get_args(tpat);
  lstpat_t *tphead = pargs[0];
  lstpat_t *tptail = pargs[1];
  lsmres_t mres = lsthunk_match_pat(thead, tphead, tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return lsthunk_match_pat(ttail, tptail, tenv);
}

/**
 * Match a thunk with a pattern
 * @param thunk The thunk (must be a list thunk)
 * @param tpat The pattern (must be a cons pattern)
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_tlist_pcons(lsthunk_t *thunk,
                                          const lstpat_t *tpat,
                                          lstenv_t *tenv) {
  // TLIST <-> TPCONS
  lsassert_tlist(thunk);
  lsassert_tpcons(tpat);
  lssize_t targc = lsthunk_get_argc(thunk);
  if (targc == 0)
    return LSMATCH_FAILURE;
  lsthunk_t *const *targs = lsthunk_get_args(thunk);
  lstpat_t *const *pargs = lstpat_get_args(tpat);
  lsmres_t mres = lsthunk_match_pat(targs[0], pargs[0], tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  lsthunk_t *trest = lsthunk_new_alge(lsconstr_list(), targc - 1, targs + 1);
  return lsthunk_match_pat(trest, pargs[1], tenv);
}

/**
 * Match a thunk with a pattern
 * @param thunk The thunk
 * @param tpat The pattern (must be a cons pattern)
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_pcons(lsthunk_t *thunk, const lstpat_t *tpat,
                                    lstenv_t *tenv) {
  // THUNK <-> TPCONS
  lsassert_tpcons(tpat);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  if (thunk_whnf == NULL)
    return LSMATCH_FAILURE;
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
  switch (ttype) {
  case LSTTYPE_ALGE:
    if (lsstrcmp(lstpat_get_constr(tpat), lsconstr_list()) == 0)
      return lsthunk_match_tlist_pcons(thunk_whnf, tpat, tenv);
    return lsthunk_match_talge_palge_simple(thunk_whnf, tpat, tenv);
  case LSTTYPE_STR:
    return lsthunk_match_tstr_pcons(thunk_whnf, tpat, tenv);
  default:
    return LSMATCH_FAILURE;
  }
}

static lsmres_t lsthunk_match_tcons_plist(lsthunk_t *thunk,
                                          const lstpat_t *tpat,
                                          lstenv_t *tenv) {
  // TCONS <-> TPLIST
  lsassert_tcons(thunk);
  lsassert_tplist(tpat);
  lssize_t pargc = lstpat_get_argc(tpat);
  if (pargc == 0)
    return LSMATCH_FAILURE;
  lsthunk_t *const *targs = lsthunk_get_args(thunk); // head : tail
  lstpat_t *const *pargs = lstpat_get_args(tpat);    // [head, ...]
  lsmres_t mres = lsthunk_match_pat(targs[0], pargs[0], tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  if (pargc == 1)
    return lsthunk_match_pat(targs[1], lstpat_get_nil(), tenv);
  lsthunk_t *trest = lsthunk_new_alge(lsconstr_list(), 1, &targs[1]);
  return lsthunk_match_pat(trest, pargs[1], tenv);
}

static lsmres_t lsthunk_match_tstr_plist(lsthunk_t *thunk, const lstpat_t *tpat,
                                         lstenv_t *tenv) {
  // TSTR <-> TPLIST
  lsassert_tstr(thunk);
  lsassert_tplist(tpat);
  lssize_t tstrlen = lsstrlen(lsthunk_get_str(thunk));
  lssize_t pargc = lstpat_get_argc(tpat);
  if (tstrlen != pargc)
    return LSMATCH_FAILURE;
  const lsstr_t *tstrval = lsthunk_get_str(thunk);
  const char *tstrbuf = lsstr_get_buf(tstrval);
  lstpat_t *const *pargs = lstpat_get_args(tpat);
  for (lssize_t i = 0; i < pargc; i++) {
    const lsint_t *int_val = lsint_new(tstrbuf[i] & 255);
    lsthunk_t *tint = lsthunk_new_int(int_val);
    lsmres_t mres = lsthunk_match_pat(tint, pargs[i], tenv);
    if (mres != LSMATCH_SUCCESS)
      return mres;
  }
  return LSMATCH_SUCCESS;
}

static lsmres_t lsthunk_match_plist(lsthunk_t *thunk, const lstpat_t *tpat,
                                    lstenv_t *tenv) {
  // THUNK <-> TPLIST
  lsassert_tpalge(tpat);
  if (lstpat_is_nil(tpat))
    return lsthunk_is_nil(thunk) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
  if (lsthunk_is_cons(thunk))
    return lsthunk_match_tcons_plist(thunk, tpat, tenv);
  if (thunk->lt_type == LSTTYPE_STR)
    return lsthunk_match_tstr_plist(thunk, tpat, tenv);
  return LSMATCH_FAILURE;
}

/**
 * Match an integer value with a thunk
 * @param thunk The thunk
 * @param tpat The integer value
 * @return The result
 */
static lsmres_t lsthunk_match_pint(lsthunk_t *thunk, const lstpat_t *tpat) {
  lsassert_tpint(tpat);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
  if (ttype != LSTTYPE_INT)
    return LSMATCH_FAILURE;
  const lsint_t *pintval = lstpat_get_int(tpat);
  const lsint_t *tintval = lsthunk_get_int(thunk_whnf);
  return lsint_eq(pintval, tintval) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

/**
 * Match a string value with a thunk
 * @param thunk The thunk
 * @param tpat The string value
 * @return The result
 */
static lsmres_t lsthunk_match_pstr(lsthunk_t *thunk, const lstpat_t *tpat) {
  // THUNK <-> TPSTR
  lsassert_tpstr(tpat);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  const lsstr_t *pstrval = lstpat_get_str(tpat);
  if (thunk_whnf->lt_type == LSTTYPE_STR) {
    // TSTR <-> TPSTR
    const lsstr_t *tstrval = lsthunk_get_str(thunk_whnf);
    return lsstrcmp(pstrval, tstrval) == 0 ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
  }
  const char *pstrbuf = lsstr_get_buf(pstrval);
  lssize_t pstrlen = lsstrlen(pstrval);
  if (lsthunk_is_cons(thunk_whnf)) {
    // TCONS <-> TPSTR
    const lsint_t *int_phead = lsint_new(pstrbuf[0] & 255);
    lstpat_t *phead = lstpat_new_int(int_phead);
    lsthunk_t *const *targs = lsthunk_get_args(thunk_whnf);
    lsthunk_t *thead = targs[0];
    lsmres_t mres = lsthunk_match_pint(thead, phead);
    if (mres != LSMATCH_SUCCESS)
      return mres;
    lstpat_t *ptail = lstpat_new_str(lsstr_sub(pstrval, 1, pstrlen - 1));
    lsthunk_t *ttail = targs[1];
    return lsthunk_match_pint(ttail, ptail);
  }
  if (lsthunk_is_list(thunk_whnf)) {
    // TLIST <-> TPSTR
    lssize_t targc = lsthunk_get_argc(thunk_whnf);
    if (pstrlen != targc)
      return LSMATCH_SUCCESS;
    lsthunk_t *const *targs = lsthunk_get_args(thunk_whnf);
    for (lssize_t i = 0; i < targc; i++) {
      const lsint_t *int_tpati = lsint_new(pstrbuf[i] & 255);
      lstpat_t *tpati = lstpat_new_int(int_tpati);
      lsthunk_t *thead = targs[i];
      lsmres_t mres = lsthunk_match_pint(thead, tpati);
      if (mres != LSMATCH_SUCCESS)
        return mres;
    }
    return LSMATCH_SUCCESS;
  }
  return LSMATCH_FAILURE;
}

/**
 * Match a thunk with a pattern
 * @param thunk The thunk
 * @param tpat The pattern
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_tcons_palge(lsthunk_t *thunk,
                                          const lstpat_t *tpat,
                                          lstenv_t *tenv) {
  // TCONS <-> TPALGE
  lsassert_tcons(thunk);
  lsassert_tpalge(tpat);
  if (lstpat_is_list(tpat))
    return lsthunk_match_tcons_plist(thunk, tpat, tenv);
  return lsthunk_match_talge_palge_simple(thunk, tpat, tenv);
}

/**
 * Match a thunk with a pattern
 * @param thunk The thunk
 * @param tpat The pattern
 * @param tenv The environment
 * @return The result
 */
static lsmres_t lsthunk_match_tlist_palge(lsthunk_t *thunk,
                                          const lstpat_t *tpat,
                                          lstenv_t *tenv) {
  // TLIST <-> TPALGE
  lsassert_tlist(thunk);
  lsassert_tpalge(tpat);
  if (lstpat_is_cons(tpat))
    return lsthunk_match_tlist_pcons(thunk, tpat, tenv);
  return lsthunk_match_talge_palge_simple(thunk, tpat, tenv);
}

/**
 * Match a thunk with a pattern
 * @param thunk The thunk
 * @param tpat The pattern
 * @param tenv The environment
 * @return The result
 */
lsmres_t lsthunk_match_palge(lsthunk_t *thunk, const lstpat_t *tpat,
                             lstenv_t *tenv) {
  // THUNK <-> TPALGE
  lsassert_tpalge(tpat);
  if (lstpat_is_cons(tpat))
    return lsthunk_match_pcons(thunk, tpat, tenv);
  if (lstpat_is_list(tpat))
    return lsthunk_match_plist(thunk, tpat, tenv);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  if (thunk_whnf == NULL)
    return LSMATCH_FAILURE;
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
  if (ttype == LSTTYPE_ALGE) {
    if (lsthunk_is_cons(thunk_whnf))
      return lsthunk_match_tcons_palge(thunk_whnf, tpat, tenv);
    if (lsthunk_is_list(thunk_whnf))
      return lsthunk_match_tlist_palge(thunk_whnf, tpat, tenv);
    return lsthunk_match_talge_palge_simple(thunk_whnf, tpat, tenv);
  }
  return LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_pas(lsthunk_t *thunk, const lstpat_t *tpat,
                           lstenv_t *tenv) {
  lsassert_tpas(tpat);
  lstpat_t *tpref = lstpat_get_asref(tpat);
  lstpat_t *tpaspattern = lstpat_get_aspattern(tpat);
  lsmres_t mres = lsthunk_match_pat(thunk, tpaspattern, tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  mres = lsthunk_match_pref(thunk, tpref, tenv);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pref(lsthunk_t *thunk, const lstpat_t *tpat,
                            lstenv_t *tenv) {
  lsassert_tpref(tpat);
  const lsref_t *pref = lstpat_get_ref(tpat);
  const lsstr_t *prefname = lsref_get_name(pref);
  lstenv_get(tenv, prefname);
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pat(lsthunk_t *thunk, const lstpat_t *tpat,
                           lstenv_t *tenv) {
  switch (lstpat_get_type(tpat)) {
  case LSPTYPE_ALGE:
    return lsthunk_match_palge(thunk, tpat, tenv);
  case LSPTYPE_AS:
    return lsthunk_match_pas(thunk, tpat, tenv);
  case LSPTYPE_INT:
    return lsthunk_match_pint(thunk, tpat);
  case LSPTYPE_STR:
    return lsthunk_match_pstr(thunk, tpat);
  case LSPTYPE_REF:
    return lsthunk_match_pref(thunk, tpat, tenv);
  }
  return LSMATCH_FAILURE;
}

lsthunk_t *lsthunk_beta_alge(lsthunk_t *thunk, lstenv_t *tenv) {
  lsassert_talge(thunk);
  lssize_t argc = lsthunk_get_argc(thunk);
  if (argc == 0)
    return thunk; // no affect beta reduction
  lsthunk_t *const *args = lsthunk_get_args(thunk);
  lssize_t count_conv = 0;
  lsthunk_t *args_new[argc];
  for (lssize_t i = 0; i < argc; i++) {
    args_new[i] = lsthunk_beta(args[i], tenv);
    if (args[i] != args_new[i])
      count_conv++;
  }
  if (count_conv == 0)
    return thunk; // no affect beta reduction
  const lsstr_t *constr = lsthunk_get_constr(thunk);
  lsthunk_t *thunk_new = lsthunk_new_alge(constr, argc, args_new);
  return thunk_new;
}

lsthunk_t *lsthunk_beta_appl(lsthunk_t *thunk, lstenv_t *tenv) {
  lsassert_tappl(thunk);
  lssize_t count_conv = 0;
  lsthunk_t *func = lsthunk_get_func(thunk);
  lsthunk_t *func_new = lsthunk_beta(func, tenv);
  if (func != func_new)
    count_conv++;
  lssize_t argc = lsthunk_get_argc(thunk);
  lsthunk_t *const *args = lsthunk_get_args(thunk);
  lsthunk_t *args_new[argc];
  for (lssize_t i = 0; i < argc; i++) {
    args_new[i] = lsthunk_beta(args[i], tenv);
    if (args[i] != args_new[i])
      count_conv++;
  }
  if (count_conv == 0)
    return thunk; // no affect beta reduction
  return lsthunk_new_appl(func_new, argc, args_new);
}

static lsthunk_t *lsthunk_beta_choice(lsthunk_t *thunk, lstenv_t *tenv) {
  lsassert_tchoice(thunk);
  lsthunk_t *left = lsthunk_get_left(thunk);
  lsthunk_t *right = lsthunk_get_right(thunk);
  lsthunk_t *left_new = lsthunk_beta(left, tenv);
  lsthunk_t *right_new = lsthunk_beta(right, tenv);
  if (left == left_new && right == right_new)
    return thunk; // no affect beta reduction
  return lsthunk_new_choice(left_new, right_new);
}

lsthunk_t *lsthunk_beta(lsthunk_t *thunk, lstenv_t *tenv) {
  lsassert(thunk != NULL);
  switch (thunk->lt_type) {
  case LSTTYPE_ALGE:
    return lsthunk_beta_alge(thunk, tenv);
  case LSTTYPE_APPL:
    return lsthunk_beta_appl(thunk, tenv);
  case LSTTYPE_CHOICE:
    return lsthunk_beta_choice(thunk, tenv);
  case LSTTYPE_INT:
    return thunk;
  case LSTTYPE_STR:
    return thunk; // MUST NOT HAPPEN
  default:
    return lsthunk_new_beta(tenv, thunk);
  }
  lsassert_notreach(); // MUST NOT HAPPEN
}

lsthunk_t *lsthunk_eval_alge(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args) {
  // eval (C a b ...) x y ... = C a b ... x y ...
  lsassert(thunk != NULL);
  lsassert(thunk->lt_type == LSTTYPE_ALGE);
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

lsthunk_t *lsthunk_eval_appl(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args) {
  // eval (f a b ...) x y ... => eval f a b ... x y ...
  lsassert(thunk != NULL);
  lsassert(thunk->lt_type == LSTTYPE_APPL);
  if (argc == 0)
    return lsthunk_eval(thunk);
  lsthunk_t *func1 = thunk->lt_appl.lta_func;
  lssize_t argc1 = thunk->lt_appl.lta_argc + argc;
  lsthunk_t *const *args1 = (lsthunk_t *const *)lsa_concata(
      thunk->lt_appl.lta_argc, (const void *const *)thunk->lt_appl.lta_args,
      argc, (const void *const *)args);
  return lsthunk_eval_thunk(func1, argc1, args1);
}

lsthunk_t *lsthunk_eval_lambda(lsthunk_t *thunk, lssize_t argc,
                               lsthunk_t *const *args) {
  // eval (\param -> body) x y ... = eval (body[param := x]) y ...
  lsassert(thunk != NULL);
  lsassert(thunk->lt_type == LSTTYPE_LAMBDA);
  lsassert(argc > 0);
  lsassert(args != NULL);
  lstpat_t *param = lsthunk_get_param(thunk);
  lsthunk_t *body = lsthunk_get_body(thunk);
  lsthunk_t *arg;
  lsthunk_t *const *args1 = (lsthunk_t *const *)lsa_shift(
      argc, (const void *const *)args, (const void **)&arg);
  lstenv_t *tenv = lstenv_new(NULL);
  lsmres_t mres = lsthunk_match_pat(arg, param, tenv);
  if (mres != LSMATCH_SUCCESS)
    return NULL;
  lsthunk_t *body_new = lsthunk_beta(body, tenv);
  if (body_new == NULL)
    return NULL;
  return lsthunk_eval_thunk(body_new, argc - 1, args1);
}

lsthunk_t *lsthunk_eval_builtin(lsthunk_t *thunk, lssize_t argc,
                                lsthunk_t *const *args) {
  lsassert_tbuiltin(thunk);
  lssize_t arity = thunk->lt_builtin.lti_arity;
  lstbuiltin_func_t func = thunk->lt_builtin.lti_func;
  void *data = thunk->lt_builtin.lti_data;
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
  return lsthunk_eval_thunk(ret, argc - arity, args + arity);
}

lsthunk_t *lsthunk_eval_choice(lsthunk_t *thunk, lssize_t argc,
                               lsthunk_t *const *args) {
  // eval (l | r) x y ... = eval l x y ... | <differed> eval r x y ...
  lsassert(thunk != NULL);
  lsassert(thunk->lt_type == LSTTYPE_CHOICE);
  lsassert_args(argc, args);
  lsthunk_t *left = lsthunk_eval_thunk(thunk->lt_choice.ltc_left, argc, args);
  if (left == NULL)
    return lsthunk_eval_thunk(thunk->lt_choice.ltc_right, argc, args);
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

lsthunk_t *lsthunk_eval_paramref(lsthunk_t *thunk, lssize_t argc,
                                 lsthunk_t *const *args) {
  lsassert_tparamref(thunk);
  lsassert_args(argc, args);
  lsthunk_t *bound = lsthunk_get_bound(thunk);
  assert(bound != NULL);
  return lsthunk_eval_thunk(bound, argc, args);
}

lsthunk_t *lsthunk_eval_thunkref(lsthunk_t *thunk, lssize_t argc,
                                 lsthunk_t *const *args) {
  lsassert_tthunkref(thunk);
  lsassert_args(argc, args);
  lsthunk_t *bound = lsthunk_get_bound(thunk);
  lsassert(bound != NULL);
  return lsthunk_eval_thunk(bound, argc, args);
}

lsthunk_t *lsthunk_eval_bindref(lsthunk_t *thunk, lssize_t argc,
                                lsthunk_t *const *args) {
  lsassert_tbindref(thunk);
  lsassert_args(argc, args);
  lsthunk_t *bound = lsthunk_get_bound(thunk);
  if (bound != NULL)
    return bound;
  // resolve reference
  lstbind_t *bind = lsthunk_get_bind(thunk);
  lsassert(bind != NULL);
  lstenv_t *env = lstbind_get_env(bind);
  if (env == NULL) {
    lstpat_t *lhs = lstbind_get_lhs(bind);
    lsthunk_t *rhs = lstbind_get_rhs(bind);
    lsmres_t mres = lsthunk_match_pat(rhs, lhs, env);
    if (mres != LSMATCH_SUCCESS)
      return NULL;
    lstbind_set_env(bind, env);
  }
  const lsref_t *bindref = lsthunk_get_ref(thunk);
  const lsstr_t *bindname = lsref_get_name(bindref);
  bound = lstenv_get(env, bindname);
  if (bound == NULL) {
    lsprintf(stderr, 0, "F: unbound reference %s\n", lsstr_get_buf(bindname));
    return NULL;
  }
  lsthunk_set_bound(thunk, bound);
  return lsthunk_eval_thunk(bound, argc, args);
}

lsthunk_t *lsthunk_eval_thunk(lsthunk_t *func, lssize_t argc,
                              lsthunk_t *const *args) {
  lsassert(func != NULL);
  if (argc == 0)
    return lsthunk_eval(func);
  switch (lsthunk_get_type(func)) {
  case LSTTYPE_ALGE:
    return lsthunk_eval_alge(func, argc, args);
  case LSTTYPE_APPL:
    return lsthunk_eval_appl(func, argc, args);
  case LSTTYPE_BETA:
    return lsthunk_eval_beta(func, argc, args);
  case LSTTYPE_BINDREF:
    return lsthunk_eval_bindref(func, argc, args);
  case LSTTYPE_BUILTIN:
    return lsthunk_eval_builtin(func, argc, args);
  case LSTTYPE_CHOICE:
    return lsthunk_eval_choice(func, argc, args);
  case LSTTYPE_INT:
    lsprintf(stderr, 0, "F: cannot apply for integer\n");
    return NULL;
  case LSTTYPE_LAMBDA:
    return lsthunk_eval_lambda(func, argc, args);
  case LSTTYPE_PARAMREF:
    return lsthunk_eval_paramref(func, argc, args);
  case LSTTYPE_STR:
    lsprintf(stderr, 0, "F: cannot apply for string\n");
    return NULL;
  case LSTTYPE_THUNKREF:
    return lsthunk_eval_thunkref(func, argc, args);
  }
}

lsthunk_t *lsprog_eval(const lsprog_t *prog, lstenv_t *tenv) {
  lsthunk_t *thunk = lsthunk_new_expr(lsprog_get_expr(prog), tenv);
  if (thunk == NULL)
    return NULL;
  return lsthunk_eval(thunk);
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
  lsassert(pcolle != NULL);
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
    thunk = lsthunk_eval(thunk);
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
    thunk = lsthunk_eval(thunk);
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
  lsassert(colle_found != NULL);

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
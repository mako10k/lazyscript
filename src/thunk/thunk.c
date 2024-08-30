#include "thunk/thunk.h"
#include "common/io.h"
#include "common/malloc.h"
#include "thunk/talge.h"
#include "thunk/tenv.h"
#include "thunk/tlambda.h"
#include "thunk/tpat.h"
#include <assert.h>
#include <stddef.h>

struct lsthunk {
  lsttype_t lt_type;
  lsthunk_t *lt_whnf;
  union {
    struct {
      union {
        const lsstr_t *lt_constr;
        lsthunk_t *lt_func;
      };
      lssize_t lt_argc;
      const lsthunk_t *lt_args[0];
    };
    struct {
      const lsint_t *lt_int;
      void *lt_intend[0];
    };
    struct {
      const lsstr_t *lt_str;
      void *lt_strend[0];
    };
    lstref_t *lt_ref;
    struct {
      const lstpat_t *lt_param;
      lsthunk_t *lt_body;
      void *lt_lambdaend[0];
    };
    struct {
      lsthunk_t *lt_left;
      lsthunk_t *lt_right;
      void *lt_choiceend[0];
    };
  };
};

lsthunk_t *lsthunk_new_alge(const lsealge_t *ealge, lstenv_t *tenv) {
  lssize_t eargc = lsealge_get_argc(ealge);
  const lsexpr_t *const *eargs = lsealge_get_args(ealge);
  lsthunk_t *thunk =
      lsmalloc(offsetof(lsthunk_t, lt_argc) + eargc * sizeof(lsthunk_t *));
  thunk->lt_type = LSTTYPE_ALGE;
  thunk->lt_whnf = NULL;
  thunk->lt_constr = lsealge_get_constr(ealge);
  thunk->lt_argc = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return thunk;
}

lsthunk_t *lsthunk_new_appl(const lseappl_t *eappl, lstenv_t *tenv) {
  lssize_t eargc = lseappl_get_argc(eappl);
  const lsexpr_t *const *eargs = lseappl_get_args(eappl);
  lsthunk_t *thunk =
      lsmalloc(offsetof(lsthunk_t, lt_argc) + eargc * sizeof(lsthunk_t *));
  thunk->lt_type = LSTTYPE_APPL;
  thunk->lt_whnf = NULL;
  thunk->lt_func = lsthunk_new_expr(lseappl_get_func(eappl), tenv);
  thunk->lt_argc = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return thunk;
}

lsthunk_t *lsthunk_new_choice(const lsechoice_t *echoice, lstenv_t *tenv) {
  lsthunk_t *thunk = lsmalloc(offsetof(lsthunk_t, lt_choiceend));
  thunk->lt_type = LSTTYPE_CHOICE;
  thunk->lt_whnf = NULL;
  thunk->lt_left = lsthunk_new_expr(lsechoice_get_left(echoice), tenv);
  thunk->lt_right = lsthunk_new_expr(lsechoice_get_right(echoice), tenv);
  return thunk;
}

lsthunk_t *lsthunk_new_closure(const lseclosure_t *eclosure, lstenv_t *tenv) {
  return NULL; // TODO : implement
}

lsthunk_t *lsthunk_new_ref(const lsref_t *ref, lstenv_t *tenv) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_REF;
  thunk->lt_whnf = NULL;
  thunk->lt_ref = lstref_new(ref, tenv); // TODO resolve the reference
  return thunk;
}

lsthunk_t *lsthunk_new_expr(const lsexpr_t *expr, lstenv_t *tenv) {
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_ALGE:
    return lsthunk_new_alge(lsexpr_get_alge(expr), tenv);
  case LSETYPE_APPL:
    return lsthunk_new_appl(lsexpr_get_appl(expr), tenv);
  case LSETYPE_CHOICE:
    return lsthunk_new_choice(lsexpr_get_choice(expr), tenv);
  case LSETYPE_CLOSURE:
    return lsthunk_new_closure(lsexpr_get_closure(expr), tenv);
  case LSETYPE_LAMBDA:
    return lsthunk_new_lambda(lsexpr_get_lambda(expr), tenv);
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

lstalge_t *lsthunk_get_alge(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge;
}

lstappl_t *lsthunk_get_appl(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_appl;
}

lstchoice_t *lsthunk_get_choice(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice;
}

lstlambda_t *lsthunk_get_lambda(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda;
}

lstref_t *lsthunk_get_ref(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_REF);
  return thunk->lt_ref;
}

const lsint_t *lsthunk_get_int(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}

const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}

lsmres_t lsthunk_match_palge(lsthunk_t *thunk, const lspalge_t *palge,
                             lstenv_t *tenv) {
  assert(palge != NULL);
  lsthunk_t *thunk_whnf = lsthunk_eval(thunk);
  lsttype_t ttype = lsthunk_get_type(thunk_whnf);
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
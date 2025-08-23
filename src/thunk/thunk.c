#include "thunk/thunk.h"
#include "common/io.h"
#include "common/loc.h"
#include "common/malloc.h"
#include "expr/eclosure.h"
#include "lstypes.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"
#include "runtime/error.h"
#include "runtime/trace.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

// Debug tracing for thunk evaluation (off by default). Enable with -DLS_TRACE=1
#ifndef LS_TRACE
#define LS_TRACE 0
#endif

struct lstalge {
  const lsstr_t* lta_constr;
  lssize_t       lta_argc;
  lsthunk_t*     lta_args[0];
};

struct lstappl {
  lsthunk_t* lta_func;
  lssize_t   lta_argc;
  lsthunk_t* lta_args[0];
};

struct lstchoice {
  lsthunk_t* ltc_left;
  lsthunk_t* ltc_right;
};

struct lstbind {
  lstpat_t*  ltb_lhs;
  lsthunk_t* ltb_rhs;
};

struct lstlambda {
  lstpat_t*  ltl_param;
  lsthunk_t* ltl_body;
};

struct lstref_target_origin {
  lstrtype_t lrto_type;
  union {
    lstbind_t   lrto_bind;
    lstlambda_t lrto_lambda;
    lsthunk_t*  lrto_builtin;
  };
};

struct lstref_target {
  lstref_target_origin_t* lrt_origin;
  lstpat_t*               lrt_pat;
};

struct lstref {
  const lsref_t*   ltr_ref;
  lstref_target_t* ltr_target;
  const lstenv_t*  ltr_env;
};

struct lstbuiltin {
  const lsstr_t*    lti_name;
  lssize_t          lti_arity;
  lstbuiltin_func_t lti_func;
  void*             lti_data;
};

struct lsthunk {
  lsttype_t  lt_type;
  lsthunk_t* lt_whnf;
  int        lt_trace_id;
  union {
    lstalge_t           lt_alge;
    lstappl_t           lt_appl;
    lstchoice_t         lt_choice;
    lstlambda_t         lt_lambda;
    lstref_t            lt_ref;
    const lsint_t*      lt_int;
    const lsstr_t*      lt_str;
  const lsstr_t*      lt_symbol;
    const lstbuiltin_t* lt_builtin;
  };
};

static int g_trace_next_id = 0;

lsthunk_t* lsthunk_new_ealge(const lsealge_t* ealge, lstenv_t* tenv) {
  lssize_t               eargc = lsealge_get_argc(ealge);
  const lsexpr_t* const* eargs = lsealge_get_args(ealge);
  lsthunk_t* thunk          = lsmalloc(lssizeof(lsthunk_t, lt_alge) + eargc * sizeof(lsthunk_t*));
  thunk->lt_type            = LSTTYPE_ALGE;
  thunk->lt_whnf            = thunk;
  thunk->lt_trace_id        = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_alge.lta_constr = lsealge_get_constr(ealge);
  thunk->lt_alge.lta_argc   = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_alge.lta_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return thunk;
}

lsthunk_t* lsthunk_new_eappl(const lseappl_t* eappl, lstenv_t* tenv) {
  lssize_t               eargc = lseappl_get_argc(eappl);
  const lsexpr_t* const* eargs = lseappl_get_args(eappl);
  lsthunk_t*             func  = lsthunk_new_expr(lseappl_get_func(eappl), tenv);
  if (func == NULL)
    return NULL;
  lsthunk_t* args_buf[eargc];
  for (lssize_t i = 0; i < eargc; i++) {
    args_buf[i] = lsthunk_new_expr(eargs[i], tenv);
    if (args_buf[i] == NULL)
      return NULL;
  }
  lsthunk_t* thunk        = lsmalloc(lssizeof(lsthunk_t, lt_appl) + eargc * sizeof(lsthunk_t*));
  thunk->lt_type          = LSTTYPE_APPL;
  thunk->lt_whnf          = NULL;
  thunk->lt_trace_id      = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_appl.lta_func = func;
  thunk->lt_appl.lta_argc = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_appl.lta_args[i] = args_buf[i];
  return thunk;
}

lsthunk_t* lsthunk_new_echoice(const lsechoice_t* echoice, lstenv_t* tenv) {
  lsthunk_t* thunk           = lsmalloc(offsetof(lsthunk_t, lt_choice));
  thunk->lt_type             = LSTTYPE_CHOICE;
  thunk->lt_whnf             = NULL;
  thunk->lt_trace_id         = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_choice.ltc_left  = lsthunk_new_expr(lsechoice_get_left(echoice), tenv);
  thunk->lt_choice.ltc_right = lsthunk_new_expr(lsechoice_get_right(echoice), tenv);
  return thunk;
}

lsthunk_t* lsthunk_new_eclosure(const lseclosure_t* eclosure, lstenv_t* tenv) {
  tenv                           = lstenv_new(tenv);
  lssize_t                ebindc = lseclosure_get_bindc(eclosure);
  const lsbind_t* const*  ebinds = lseclosure_get_binds(eclosure);
  lstref_target_origin_t* origins[ebindc];
  for (lssize_t i = 0; i < ebindc; i++) {
    const lspat_t* lhs            = lsbind_get_lhs(ebinds[i]);
    origins[i]                    = lsmalloc(sizeof(lstref_target_origin_t));
    origins[i]->lrto_type         = LSTRTYPE_BIND;
    origins[i]->lrto_bind.ltb_lhs = lstpat_new_pat(lhs, tenv, origins[i]);
    if (origins[i]->lrto_bind.ltb_lhs == NULL)
      return NULL;
  }
  for (lssize_t i = 0; i < ebindc; i++) {
    const lsexpr_t* rhs           = lsbind_get_rhs(ebinds[i]);
    origins[i]->lrto_bind.ltb_rhs = lsthunk_new_expr(rhs, tenv);
    if (origins[i]->lrto_bind.ltb_rhs == NULL)
      return NULL;
  }
  return lsthunk_new_expr(lseclosure_get_expr(eclosure), tenv);
}

lsthunk_t* lsthunk_new_ref(const lsref_t* ref, lstenv_t* tenv) {
  lstref_target_t* target  = lstenv_get(tenv, lsref_get_name(ref));
  lsthunk_t*       thunk   = lsmalloc(lssizeof(lsthunk_t, lt_ref));
  thunk->lt_type           = LSTTYPE_REF;
  thunk->lt_whnf           = NULL;
  thunk->lt_trace_id       = g_trace_next_id++;
  thunk->lt_ref.ltr_ref    = ref;
  thunk->lt_ref.ltr_target = target; // may be NULL; resolve lazily at eval
  thunk->lt_ref.ltr_env    = tenv;
  // Prefer pending loc; fallback to ref's own loc
  {
    lsloc_t loc = lstrace_take_pending_or_unknown();
    if (loc.filename && strcmp(loc.filename, "<unknown>") != 0) lstrace_emit_loc(loc);
    else lstrace_emit_loc(lsref_get_loc(ref));
  }
  return thunk;
}

lsthunk_t* lsthunk_new_int(const lsint_t* intval) {
  lsthunk_t* thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type   = LSTTYPE_INT;
  thunk->lt_whnf   = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  thunk->lt_int    = intval;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  return thunk;
}

lsthunk_t* lsthunk_new_str(const lsstr_t* strval) {
  lsthunk_t* thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type   = LSTTYPE_STR;
  thunk->lt_whnf   = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  thunk->lt_str    = strval;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  return thunk;
}

lsthunk_t* lsthunk_new_symbol(const lsstr_t* sym) {
  lsthunk_t* thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type   = LSTTYPE_SYMBOL;
  thunk->lt_whnf   = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  thunk->lt_symbol = sym;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  return thunk;
}

lsthunk_t* lsthunk_new_elambda(const lselambda_t* elambda, lstenv_t* tenv) {
  const lspat_t*  pparam         = lselambda_get_param(elambda);
  const lsexpr_t* ebody          = lselambda_get_body(elambda);
  tenv                           = lstenv_new(tenv);
  lstref_target_origin_t* origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type              = LSTRTYPE_LAMBDA;
  origin->lrto_lambda.ltl_param  = lstpat_new_pat(pparam, tenv, origin);
  if (origin->lrto_lambda.ltl_param == NULL)
    return NULL;
  origin->lrto_lambda.ltl_body = lsthunk_new_expr(ebody, tenv);
  if (origin->lrto_lambda.ltl_body == NULL)
    return NULL;
  lsthunk_t* thunk           = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type             = LSTTYPE_LAMBDA;
  thunk->lt_whnf             = thunk;
  thunk->lt_trace_id         = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_lambda.ltl_param = origin->lrto_lambda.ltl_param;
  thunk->lt_lambda.ltl_body  = origin->lrto_lambda.ltl_body;
  return thunk;
}

lsthunk_t* lsthunk_new_expr(const lsexpr_t* expr, lstenv_t* tenv) {
  // Record pending loc for this expression so JSONL emission uses it
  lstrace_set_pending_loc(lsexpr_get_loc(expr));
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_ALGE:
    {
      const lsealge_t* ealge = lsexpr_get_alge(expr);
      // If this is a zero-arity constructor and the constructor name starts with '.', treat as Symbol
      if (lsealge_get_argc(ealge) == 0) {
        const lsstr_t* c = lsealge_get_constr(ealge);
        const char*    s = lsstr_get_buf(c);
        if (s && s[0] == '.')
          return lsthunk_new_symbol(c);
      }
      return lsthunk_new_ealge(ealge, tenv);
    }
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

lsttype_t lsthunk_get_type(const lsthunk_t* thunk) {
  assert(thunk != NULL);
  return thunk->lt_type;
}

const lsstr_t* lsthunk_get_constr(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge.lta_constr;
}

lsthunk_t* lsthunk_get_func(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_appl.lta_func;
}

lssize_t lsthunk_get_argc(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_argc : thunk->lt_appl.lta_argc;
}

lsthunk_t* const* lsthunk_get_args(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_args : thunk->lt_appl.lta_args;
}

lsthunk_t* lsthunk_get_left(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice.ltc_left;
}

lsthunk_t* lsthunk_get_right(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  return thunk->lt_choice.ltc_right;
}

lstpat_t* lsthunk_get_param(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_param;
}

lsthunk_t* lsthunk_get_body(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_body;
}

lstref_target_t* lsthunk_get_ref_target(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_REF);
  return thunk->lt_ref.ltr_target;
}

const lsint_t* lsthunk_get_int(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}

const lsstr_t* lsthunk_get_str(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}

const lsstr_t* lsthunk_get_symbol(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_SYMBOL);
  return thunk->lt_symbol;
}

lsmres_t lsthunk_match_alge(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_ALGE);
  lsthunk_t* thunk_whnf = lsthunk_eval0(thunk);
  lsttype_t  ttype      = lsthunk_get_type(thunk_whnf);
  if (ttype != LSTTYPE_ALGE)
    return LSMATCH_FAILURE; // TODO: match as list or string
  const lsstr_t* tconstr = lsthunk_get_constr(thunk_whnf);
  const lsstr_t* pconstr = lstpat_get_constr(tpat);
  if (lsstrcmp(pconstr, tconstr) != 0)
    return LSMATCH_FAILURE;
  lssize_t pargc = lstpat_get_argc(tpat);
  lssize_t targc = lsthunk_get_argc(thunk_whnf);
  if (pargc != targc)
    return LSMATCH_FAILURE;
  lstpat_t* const*  pargs = lstpat_get_args(tpat);
  lsthunk_t* const* targs = lsthunk_get_args(thunk_whnf);
  for (lssize_t i = 0; i < pargc; i++)
    if (lsthunk_match_pat(targs[i], pargs[i]) < 0)
      return LSMATCH_FAILURE;
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pas(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_AS);
  lstpat_t* tpref       = lstpat_get_ref(tpat);
  lstpat_t* tpaspattern = lstpat_get_aspattern(tpat);
  lsmres_t  mres        = lsthunk_match_pat(thunk, tpaspattern);
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
static lsmres_t lsthunk_match_int(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_INT);
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_INT)
    return LSMATCH_FAILURE;
  const lsint_t* pintval = lstpat_get_int(tpat);
  const lsint_t* tintval = lsthunk_get_int(thunk);
  return lsint_eq(pintval, tintval) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

/**
 * Match a string value with a thunk
 * @param thunk The thunk
 * @param tpat The string value
 * @return The result
 */
static lsmres_t lsthunk_match_str(lsthunk_t* thunk, const lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_STR);
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_STR)
    return LSMATCH_FAILURE; // TODO: match as list
  const lsstr_t* pstrval = lstpat_get_str(tpat);
  const lsstr_t* tstrval = lsthunk_get_str(thunk);
  return lsstrcmp(pstrval, tstrval) == 0 ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_ref(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_REF);
  lstpat_set_refbound(tpat, thunk);
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pat(lsthunk_t* thunk, lstpat_t* tpat) {
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
  case LSPTYPE_WILDCARD:
    return LSMATCH_SUCCESS;
  case LSPTYPE_OR: {
    // Try left; on failure, clear any ref bindings and try right
    lstpat_t* left  = lstpat_get_or_left(tpat);
    lstpat_t* right = lstpat_get_or_right(tpat);
    lsmres_t  mres  = lsthunk_match_pat(thunk, left);
    if (mres == LSMATCH_SUCCESS)
      return LSMATCH_SUCCESS;
    lstpat_clear_binds(left);
    return lsthunk_match_pat(thunk, right);
  }
  }
  return LSMATCH_FAILURE;
}

static lsthunk_t* lsthunk_eval_alge(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (C a b ...) x y ... = C a b ... x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_ALGE);
  if (argc == 0)
    return thunk; // it already is in WHNF
  lssize_t   targc = lsthunk_get_argc(thunk);
  // Allocate enough space for existing args + new args
  lsthunk_t* thunk_new =
      lsmalloc(lssizeof(lsthunk_t, lt_alge) + (targc + argc) * sizeof(lsthunk_t*));
  thunk_new->lt_type            = LSTTYPE_ALGE;
  // This node is already in WHNF; point to self, not the old thunk
  thunk_new->lt_whnf            = thunk_new;
  thunk_new->lt_trace_id        = thunk->lt_trace_id;
  thunk_new->lt_alge.lta_constr = thunk->lt_alge.lta_constr;
  thunk_new->lt_alge.lta_argc   = targc + argc;
  for (lssize_t i = 0; i < targc; i++)
    thunk_new->lt_alge.lta_args[i] = thunk->lt_alge.lta_args[i];
  for (lssize_t i = 0; i < argc; i++)
    thunk_new->lt_alge.lta_args[targc + i] = args[i];
  return thunk_new;
}

static lsthunk_t* lsthunk_eval_appl(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (f a b ...) x y ... => eval f a b ... x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_APPL);
  if (argc == 0)
    return lsthunk_eval0(thunk);
  lsthunk_t*        func1 = thunk->lt_appl.lta_func;
  lssize_t          argc1 = thunk->lt_appl.lta_argc + argc;
  lsthunk_t* const* args1 = (lsthunk_t* const*)lsa_concata(
      thunk->lt_appl.lta_argc, (const void* const*)thunk->lt_appl.lta_args, argc,
      (const void* const*)args);
  return lsthunk_eval(func1, argc1, args1);
}

static lsthunk_t* lsthunk_eval_lambda(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (\param -> body) x y ... = eval (body[param := x]) y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  assert(argc > 0);
  assert(args != NULL);
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG lambda: apply argc=%ld\n", (long)argc);
  #endif
  lstpat_t*         param = lsthunk_get_param(thunk);
  lsthunk_t*        body  = lsthunk_get_body(thunk);
  lsthunk_t*        arg;
  lsthunk_t* const* args1 =
      (lsthunk_t* const*)lsa_shift(argc, (const void* const*)args, (const void**)&arg);
  if (arg) {
    #if LS_TRACE
    const char* at = "?";
    switch (lsthunk_get_type(arg)) {
    case LSTTYPE_INT: at = "int"; break; case LSTTYPE_STR: at = "str"; break;
    case LSTTYPE_ALGE: at = "alge"; break; case LSTTYPE_APPL: at = "appl"; break;
    case LSTTYPE_LAMBDA: at = "lambda"; break; case LSTTYPE_REF: at = "ref"; break;
    case LSTTYPE_CHOICE: at = "choice"; break; case LSTTYPE_BUILTIN: at = "builtin"; break; }
    lsprintf(stderr, 0, "DBG lambda: arg type=%s\n", at);
    #endif
  }
  // Bind directly on the lambda's original parameter pattern so that
  // references inside body (which point to the same pattern objects via env)
  // observe the binding.
  body          = lsthunk_clone(body);
  lsmres_t mres = lsthunk_match_pat(arg, param);
  if (mres != LSMATCH_SUCCESS) {
    #if LS_TRACE
    lsprintf(stderr, 0, "DBG lambda: match failed\n");
    #endif
    return ls_make_err("lambda match failure");
  }
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG lambda: eval body\n");
  #endif
  lsthunk_t* ret = lsthunk_eval(body, argc - 1, args1);
  if (ret == NULL) {
    #if LS_TRACE
    lsprintf(stderr, 0, "DBG lambda: body eval returned NULL\n");
    #endif
    return NULL;
  }
  // Capture current binding of this parameter inside the returned value by
  // substituting any references to the parameter with the bound thunk. This
  // prevents subsequent applications of the same lambda from mutating the
  // shared parameter pattern and affecting already-produced values.
  extern lsthunk_t* lsthunk_subst_param(lsthunk_t* thunk, lstpat_t* param);
  ret = lsthunk_subst_param(ret, param);
  // Clear the parameter bindings now that the value has captured them.
  lstpat_clear_binds(param);
  if (ret) {
    #if LS_TRACE
    const char* rt = "?";
    switch (lsthunk_get_type(ret)) {
    case LSTTYPE_INT: rt = "int"; break; case LSTTYPE_STR: rt = "str"; break;
    case LSTTYPE_ALGE: rt = "alge"; break; case LSTTYPE_APPL: rt = "appl"; break;
    case LSTTYPE_LAMBDA: rt = "lambda"; break; case LSTTYPE_REF: rt = "ref"; break;
    case LSTTYPE_CHOICE: rt = "choice"; break; case LSTTYPE_BUILTIN: rt = "builtin"; break; }
    lsprintf(stderr, 0, "DBG lambda: ret type=%s\n", rt);
    #endif
  }
  return ret;
}

static lsthunk_t* lsthunk_eval_builtin(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_BUILTIN);
  #if LS_TRACE
  const lsstr_t* bname = lsthunk_get_builtin_name(thunk);
  lsprintf(stderr, 0, "DBG builtin: call "); if (bname) lsstr_print_bare(stderr, LSPREC_LOWEST, 0, bname); else lsprintf(stderr, 0, "<anon>"); lsprintf(stderr, 0, " argc=%ld (arity=%ld)\n", (long)argc, (long)thunk->lt_builtin->lti_arity);
  #endif
  if (argc == 0) {
    // For zero-arity builtins, invoke immediately to obtain the value.
    if (thunk->lt_builtin->lti_arity == 0) {
      lstbuiltin_func_t f0   = thunk->lt_builtin->lti_func;
      void*             d0   = thunk->lt_builtin->lti_data;
      lsthunk_t*        ret0 = f0(0, NULL, d0);
      return ret0 ? lsthunk_eval0(ret0) : ls_make_err("builtin: null");
    }
    // Otherwise, it's a function waiting for more args.
    return thunk;
  }
  lssize_t          arity = thunk->lt_builtin->lti_arity;
  lstbuiltin_func_t func  = thunk->lt_builtin->lti_func;
  void*             data  = thunk->lt_builtin->lti_data;
  if (argc < arity) {
    lsthunk_t* ret        = lsmalloc(lssizeof(lsthunk_t, lt_appl) + argc * sizeof(lsthunk_t*));
    ret->lt_type          = LSTTYPE_APPL;
    ret->lt_whnf          = ret;
    ret->lt_appl.lta_func = thunk;
    ret->lt_appl.lta_argc = argc;
    for (lssize_t i = 0; i < argc; i++)
      ret->lt_appl.lta_args[i] = args[i];
    return ret;
  }
  lsthunk_t* ret = func(arity, args, data);
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG builtin: "); if (bname) lsstr_print_bare(stderr, LSPREC_LOWEST, 0, bname); else lsprintf(stderr, 0, "<anon>"); lsprintf(stderr, 0, " returned %s\n", ret ? "ok" : "NULL");
  #endif
  if (ret == NULL)
    return ls_make_err("builtin: null");
  if (lsthunk_is_err(ret))
    return ret;
  return lsthunk_eval(ret, argc - arity, args + arity);
}

static lsthunk_t* lsthunk_eval_ref(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval ~r x y ... = eval (eval ~r) x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_REF);
  assert(argc == 0 || args != NULL);
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG ref: begin\n");
  #endif
  lstref_target_t* target = thunk->lt_ref.ltr_target;
  if (target == NULL) {
    // try lazy lookup in environment captured at construction
    target = lstenv_get(thunk->lt_ref.ltr_env, lsref_get_name(thunk->lt_ref.ltr_ref));
    if (target == NULL) {
      lsprintf(stderr, 0, "E: ");
      lsloc_print(stderr, lsref_get_loc(thunk->lt_ref.ltr_ref));
      lsprintf(stderr, 0, "undefined reference: ");
      lsref_print(stderr, LSPREC_LOWEST, 0, thunk->lt_ref.ltr_ref);
      lsprintf(stderr, 0, "\n");
      // Optional eager trace stack print while stack is still active (test hook)
      const char* eager = getenv("LAZYSCRIPT_TRACE_EAGER_PRINT");
      if (eager && *eager && g_lstrace_table) {
        int depth = 1;
        const char* d = getenv("LAZYSCRIPT_TRACE_STACK_DEPTH");
        if (d && *d) { int v = atoi(d); if (v >= 0) depth = v; }
        lstrace_print_stack(stderr, depth);
        // lstrace_print_stack starts each frame with "\n at ", so no extra newline needed
      }
      return ls_make_err("undefined reference");
    }
    thunk->lt_ref.ltr_target = target; // cache
  }
  lstref_target_origin_t* origin = target->lrt_origin;
  assert(origin != NULL);
  lstpat_t* pat_ref = target->lrt_pat;
  assert(pat_ref != NULL);
  lsthunk_t* refbound = lstpat_get_refbound(pat_ref);
  if (refbound != NULL)
    return lsthunk_eval(refbound, argc, args);
  switch (origin->lrto_type) {
  case LSTRTYPE_BIND: {
    lsmres_t mres = lsthunk_match_pat(origin->lrto_bind.ltb_rhs, origin->lrto_bind.ltb_lhs);
    if (mres != LSMATCH_SUCCESS)
      return ls_make_err("ref match failure");
    refbound = lstpat_get_refbound(pat_ref);
    assert(refbound != NULL);
    return lsthunk_eval(refbound, argc, args);
  }
  case LSTRTYPE_LAMBDA: {
    // Parameter refs should have been bound during lambda application.
    refbound = lstpat_get_refbound(pat_ref);
    if (refbound == NULL) {
      lsprintf(stderr, 0, "E: unbound lambda parameter reference\n");
      return ls_make_err("unbound lambda param");
    }
    return lsthunk_eval(refbound, argc, args);
  }
  case LSTRTYPE_BUILTIN:
    return lsthunk_eval_builtin(origin->lrto_builtin, argc, args);
  }
}

static lsthunk_t* lsthunk_eval_choice(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (l | r) x y ...
  //   = let v = eval l x y ... in
  //       if v is lambda match failure then eval r x y ...
  //       else v
  // This makes '|' behave like pattern-lambda alternatives rather than
  // producing a residual choice value.
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  assert(argc == 0 || args != NULL);
  lsthunk_t* left = lsthunk_eval(thunk->lt_choice.ltc_left, argc, args);
  if (left == NULL)
    return lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
  if (lsthunk_is_err(left)) {
    // Only treat lambda match failure as a fallback trigger; propagate others.
    int is_lam_match_fail = 0;
    if (left->lt_type == LSTTYPE_ALGE &&
        lsstrcmp(left->lt_alge.lta_constr, lsstr_cstr("#err")) == 0 &&
        left->lt_alge.lta_argc == 1) {
      lsthunk_t* msgt = lsthunk_eval0(left->lt_alge.lta_args[0]);
      if (msgt && lsthunk_get_type(msgt) == LSTTYPE_STR) {
        const lsstr_t* s = lsthunk_get_str(msgt);
        if (lsstrcmp(s, lsstr_cstr("lambda match failure")) == 0)
          is_lam_match_fail = 1;
      }
    }
    if (is_lam_match_fail)
      return lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
    return left;
  }
  // Success on left: commit left-biased result.
  return left;
}

lsthunk_t* lsthunk_eval(lsthunk_t* func, lssize_t argc, lsthunk_t* const* args) {
  assert(func != NULL);
  // Enter trace context for this evaluation
  if (func->lt_trace_id >= 0) lstrace_push(func->lt_trace_id);
  if (lsthunk_is_err(func)) {
    if (func->lt_trace_id >= 0) lstrace_pop();
    return func;
  }
  if (argc == 0)
    { lsthunk_t* r = lsthunk_eval0(func); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  switch (lsthunk_get_type(func)) {
  case LSTTYPE_ALGE:
    { lsthunk_t* r = lsthunk_eval_alge(func, argc, args); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  case LSTTYPE_APPL:
    { lsthunk_t* r = lsthunk_eval_appl(func, argc, args); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  case LSTTYPE_LAMBDA:
    { lsthunk_t* r = lsthunk_eval_lambda(func, argc, args); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  case LSTTYPE_REF:
    { lsthunk_t* r = lsthunk_eval_ref(func, argc, args); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  case LSTTYPE_CHOICE:
    { lsthunk_t* r = lsthunk_eval_choice(func, argc, args); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  case LSTTYPE_INT:
    lsprintf(stderr, 0, "F: cannot apply for integer\n");
    if (func->lt_trace_id >= 0) lstrace_pop(); return NULL;
  case LSTTYPE_STR:
    lsprintf(stderr, 0, "F: cannot apply for string\n");
    if (func->lt_trace_id >= 0) lstrace_pop(); return NULL;
  case LSTTYPE_SYMBOL:
    lsprintf(stderr, 0, "F: cannot apply for symbol\n");
    if (func->lt_trace_id >= 0) lstrace_pop(); return NULL;
  case LSTTYPE_BUILTIN:
    { lsthunk_t* r = lsthunk_eval_builtin(func, argc, args); if (func->lt_trace_id >= 0) lstrace_pop(); return r; }
  }
  if (func->lt_trace_id >= 0) lstrace_pop();
  return NULL;
}

lsthunk_t* lsthunk_eval0(lsthunk_t* thunk) {
  assert(thunk != NULL);
  if (thunk->lt_whnf != NULL)
    return thunk->lt_whnf;
  if (thunk->lt_trace_id >= 0) lstrace_push(thunk->lt_trace_id);
  switch (thunk->lt_type) {
  case LSTTYPE_APPL:
    thunk->lt_whnf =
        lsthunk_eval(thunk->lt_appl.lta_func, thunk->lt_appl.lta_argc, thunk->lt_appl.lta_args);
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
  if (thunk->lt_trace_id >= 0) lstrace_pop();
  return thunk->lt_whnf;
}

lstref_target_t* lstref_target_new(lstref_target_origin_t* origin, lstpat_t* tpat) {
  lstref_target_t* target = lsmalloc(sizeof(lstref_target_t));
  target->lrt_origin      = origin;
  target->lrt_pat         = tpat;
  return target;
}

lstpat_t*  lstref_target_get_pat(lstref_target_t* target) { return target->lrt_pat; }

lsthunk_t* lsthunk_new_builtin(const lsstr_t* name, lssize_t arity, lstbuiltin_func_t func,
                               void* data) {
  lsthunk_t* thunk      = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type        = LSTTYPE_BUILTIN;
  // Do not mark builtins as WHNF at construction. This allows eval0 to
  // execute zero-arity builtins and cache their resulting value, keeping
  // wrappers (e.g., namespace member getters) transparent when printing.
  thunk->lt_whnf        = NULL;
  thunk->lt_trace_id    = -1;
  thunk->lt_trace_id    = -1;
  lstbuiltin_t* builtin = lsmalloc(sizeof(lstbuiltin_t));
  builtin->lti_name     = name;
  builtin->lti_arity    = arity;
  builtin->lti_func     = func;
  builtin->lti_data     = data;
  thunk->lt_builtin     = builtin;
  return thunk;
}

lstref_target_origin_t* lstref_target_origin_new_builtin(const lsstr_t* name, lssize_t arity,
                                                         lstbuiltin_func_t func, void* data) {
  lstref_target_origin_t* origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type              = LSTRTYPE_BUILTIN;
  origin->lrto_builtin           = lsthunk_new_builtin(name, arity, func, data);
  return origin;
}

int lsthunk_is_builtin(const lsthunk_t* thunk) {
  return thunk && thunk->lt_type == LSTTYPE_BUILTIN;
}

lstbuiltin_func_t lsthunk_get_builtin_func(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BUILTIN) ? thunk->lt_builtin->lti_func : NULL;
}

void* lsthunk_get_builtin_data(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BUILTIN) ? thunk->lt_builtin->lti_data : NULL;
}

const lsstr_t* lsthunk_get_builtin_name(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BUILTIN) ? thunk->lt_builtin->lti_name : NULL;
}

lsthunk_t* lsprog_eval(const lsprog_t* prog, lstenv_t* tenv) {
  lsthunk_t* thunk = lsthunk_new_expr(lsprog_get_expr(prog), tenv);
  if (thunk == NULL)
    return NULL;
  return lsthunk_eval0(thunk);
}

typedef enum lsprint_mode { LSPM_SHARROW, LSPM_ASIS, LSPM_DEEP } lsprint_mode_t;

typedef struct lsthunk_colle {
  // thunk entry
  lsthunk_t* ltc_thunk;
  // thunk id
  lssize_t ltc_id;
  // duplicated count
  lssize_t ltc_count;
  // minimal level
  lssize_t ltc_level;
  // next thunk entry
  struct lsthunk_colle* ltc_next;
} lsthunk_colle_t;

lsthunk_colle_t** lsthunk_colle_find(lsthunk_colle_t** pcolle, const lsthunk_t* thunk) {
  assert(pcolle != NULL);
  while (*pcolle != NULL) {
    if ((*pcolle)->ltc_thunk == thunk)
      break;
    pcolle = &(*pcolle)->ltc_next;
  }
  return pcolle;
}

static lsthunk_colle_t* lsthunk_colle_new(lsthunk_t* thunk, lsthunk_colle_t* head, lssize_t* pid,
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
  lsthunk_colle_t** pcolle = lsthunk_colle_find(&head, thunk);
  if (*pcolle != NULL) {
    (*pcolle)->ltc_id = (*pid)++;
    (*pcolle)->ltc_count++;
    if ((*pcolle)->ltc_level > level)
      (*pcolle)->ltc_level = level;
    return head;
  }
  *pcolle              = lsmalloc(sizeof(lsthunk_colle_t));
  (*pcolle)->ltc_thunk = thunk;
  (*pcolle)->ltc_id    = 0;
  (*pcolle)->ltc_count = 1;
  (*pcolle)->ltc_level = level;
  (*pcolle)->ltc_next  = NULL;
  switch (thunk->lt_type) {
  case LSTTYPE_ALGE:
    for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++)
      head = lsthunk_colle_new(thunk->lt_alge.lta_args[i], head, pid, level + 1, mode);
    break;
  case LSTTYPE_APPL:
    head = lsthunk_colle_new(thunk->lt_appl.lta_func, head, pid, level + 1, mode);
    for (lssize_t i = 0; i < thunk->lt_appl.lta_argc; i++)
      head = lsthunk_colle_new(thunk->lt_appl.lta_args[i], head, pid, level + 1, mode);
    break;
  case LSTTYPE_CHOICE:
    head = lsthunk_colle_new(thunk->lt_choice.ltc_left, head, pid, level + 1, mode);
    head = lsthunk_colle_new(thunk->lt_choice.ltc_right, head, pid, level + 1, mode);
    break;
  case LSTTYPE_LAMBDA:
    head = lsthunk_colle_new(thunk->lt_lambda.ltl_body, head, pid, level + 1, mode);
    break;
  case LSTTYPE_REF:
  case LSTTYPE_INT:
  case LSTTYPE_STR:
  case LSTTYPE_SYMBOL:
  case LSTTYPE_BUILTIN:
    break;
  }
  return head;
}

static void lsthunk_print_internal(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk,
                                   lssize_t level, lsthunk_colle_t* colle, lsprint_mode_t mode,
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
  int              has_dup     = 0;
  lsthunk_colle_t* colle_found = NULL;
  for (lsthunk_colle_t* c = colle; c != NULL; c = c->ltc_next) {
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
      // Pretty-print proper lists (x : y : [] => [x, y])
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
          thunk->lt_alge.lta_argc == 2) {
        // Collect elements along cons-chain with a safety cap
        lssize_t    cap = 8, n = 0;
        lsthunk_t** elems = lsmalloc(sizeof(lsthunk_t*) * cap);
        const lsthunk_t* cur = thunk;
        const lssize_t   max_steps = 4096;
        lssize_t         steps     = 0;
        while (1) {
          if (steps++ > max_steps) { n = -1; break; }
          const lsthunk_t* curv = lsthunk_eval0((lsthunk_t*)cur);
          if (!(curv->lt_type == LSTTYPE_ALGE &&
                lsstrcmp(curv->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
                curv->lt_alge.lta_argc == 2))
            break;
          if (n >= cap) {
            cap *= 2;
            lsthunk_t** tmp = lsmalloc(sizeof(lsthunk_t*) * cap);
            for (lssize_t i = 0; i < n; i++) tmp[i] = elems[i];
            elems = tmp;
          }
          elems[n++] = curv->lt_alge.lta_args[0];
          cur        = curv->lt_alge.lta_args[1];
        }
        if (n >= 0) {
          const lsthunk_t* tailv = lsthunk_eval0((lsthunk_t*)cur);
          int is_nil = (tailv->lt_type == LSTTYPE_ALGE &&
                        lsstrcmp(tailv->lt_alge.lta_constr, lsstr_cstr("[]")) == 0 &&
                        tailv->lt_alge.lta_argc == 0);
          if (is_nil) {
            // Render as [e0, e1, ...]
            lsprintf(fp, 0, "[");
            for (lssize_t i = 0; i < n; i++) {
              if (i > 0) lsprintf(fp, 0, ", ");
              lsthunk_t* e = lsthunk_eval0(elems[i]);
              if (e->lt_type == LSTTYPE_INT) {
                lsint_print(fp, LSPREC_LOWEST, indent, e->lt_int);
              } else if (e->lt_type == LSTTYPE_STR) {
                lsstr_print_bare(fp, LSPREC_LOWEST, indent, e->lt_str);
              } else if (e->lt_type == LSTTYPE_ALGE &&
                         lsstrcmp(e->lt_alge.lta_constr, lsstr_cstr("()")) == 0 &&
                         e->lt_alge.lta_argc == 0) {
                lsprintf(fp, 0, "()");
              } else {
                lsthunk_print_internal(fp, LSPREC_LOWEST, indent, e, level + 1, colle, mode, 0);
              }
            }
            lsprintf(fp, 0, "]");
            return;
          }
        }
        // else fallthrough to default ':' printing below
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr("[]")) == 0) {
        lsprintf(fp, 0, "[");
        for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
          if (i > 0)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent, thunk->lt_alge.lta_args[i], level + 1,
                                 colle, mode, 0);
        }
        lsprintf(fp, 0, "]");
        return;
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(",")) == 0) {
        lsprintf(fp, 0, "(");
        for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
          if (i > 0)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent, thunk->lt_alge.lta_args[i], level + 1,
                                 colle, mode, 0);
        }
        lsprintf(fp, 0, ")");
        return;
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
          thunk->lt_alge.lta_argc == 2) {
        if (prec > LSPREC_CONS)
          lsprintf(fp, 0, "(");
        lsthunk_print_internal(fp, LSPREC_CONS + 1, indent, thunk->lt_alge.lta_args[0], level + 1,
                               colle, mode, 0);
        lsprintf(fp, 0, " : ");
        lsthunk_print_internal(fp, LSPREC_CONS, indent, thunk->lt_alge.lta_args[1], level + 1,
                               colle, mode, 0);
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
        lsthunk_print_internal(fp, LSPREC_APPL + 1, indent, thunk->lt_alge.lta_args[i], level + 1,
                               colle, mode, 0);
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, 0, ")");
      break;
    case LSTTYPE_APPL:
      if (thunk->lt_appl.lta_argc == 0) {
        lsthunk_print_internal(fp, prec, indent, thunk->lt_appl.lta_func, level + 1, colle, mode,
                               0);
        return;
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, 0, "(");
      lsthunk_print(fp, LSPREC_APPL + 1, indent, thunk->lt_appl.lta_func);
      for (lssize_t i = 0; i < thunk->lt_appl.lta_argc; i++) {
        lsprintf(fp, indent, " ");
        lsthunk_print_internal(fp, LSPREC_APPL + 1, indent, thunk->lt_appl.lta_args[i], level + 1,
                               colle, mode, 0);
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, indent, ")");
      break;
    case LSTTYPE_CHOICE:
      if (prec > LSPREC_CHOICE)
        lsprintf(fp, 0, "(");
      lsthunk_print_internal(fp, LSPREC_CHOICE + 1, indent, thunk->lt_choice.ltc_left, level + 1,
                             colle, mode, 0);
      lsprintf(fp, 0, " | ");
      lsthunk_print_internal(fp, LSPREC_CHOICE, indent, thunk->lt_choice.ltc_right, level + 1,
                             colle, mode, 0);
      if (prec > LSPREC_CHOICE)
        lsprintf(fp, 0, ")");
      break;
    case LSTTYPE_LAMBDA:
      if (prec > LSPREC_LAMBDA)
        lsprintf(fp, indent, "(");
      lsprintf(fp, indent, "\\");
      lstpat_print(fp, LSPREC_APPL + 1, indent, thunk->lt_lambda.ltl_param);
      lsprintf(fp, indent, " -> ");
      lsthunk_print_internal(fp, LSPREC_LAMBDA, indent, thunk->lt_lambda.ltl_body, level + 1, colle,
                             mode, 0);
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
    case LSTTYPE_SYMBOL:
      // Print symbol as bare (already includes leading dot)
      lsstr_print_bare(fp, prec, indent, thunk->lt_symbol);
      break;
    case LSTTYPE_BUILTIN:
      lsprintf(fp, indent, "~%s{-builtin/%d-}", lsstr_get_buf(thunk->lt_builtin->lti_name),
               thunk->lt_builtin->lti_arity);
      break;
    }
  if (has_dup) {
    for (lsthunk_colle_t* c = colle; c != NULL; c = c->ltc_next) {
      if (c->ltc_level == level && c->ltc_count > 1) {
        lsprintf(fp, indent, ";\n~__ref%u = ", c->ltc_id);
        lsthunk_print_internal(fp, LSPREC_LOWEST, indent, c->ltc_thunk, level + 1, colle, mode, 1);
      }
    }
    lsprintf(fp, --indent, "\n)");
  }
}

void lsthunk_print(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk) {
  lssize_t         id    = 0;
  lsthunk_colle_t* colle = lsthunk_colle_new(thunk, NULL, &id, 0, 0);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_SHARROW, 0);
}

void lsthunk_dprint(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk) {
  lssize_t         id    = 0;
  lsthunk_colle_t* colle = lsthunk_colle_new(thunk, NULL, &id, 0, 1);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_ASIS, 0);
}

void lsthunk_deep_print(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk) {
  lssize_t         id    = 0;
  lsthunk_colle_t* colle = lsthunk_colle_new(thunk, NULL, &id, 0, 2);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_DEEP, 0);
}

lsthunk_t* lsthunk_clone(lsthunk_t* thunk) {
  // Thunks are treated as immutable graph nodes managed by GC.
  // Returning the same pointer is sufficient for now.
  return thunk;
}

// --- Capture substitution -------------------------------------------------

typedef struct subst_entry {
  const lsthunk_t* key;
  lsthunk_t*       val;
  struct subst_entry* next;
} subst_entry_t;

static lsthunk_t* subst_lookup(subst_entry_t* m, const lsthunk_t* k) {
  for (; m; m = m->next) if (m->key == k) return m->val;
  return NULL;
}

static subst_entry_t* subst_bind(subst_entry_t* m, const lsthunk_t* k, lsthunk_t* v) {
  subst_entry_t* e = lsmalloc(sizeof(subst_entry_t));
  e->key = k; e->val = v; e->next = m; return e;
}

static lsthunk_t* lsthunk_subst_param_rec(lsthunk_t* t, lstpat_t* param, subst_entry_t** pmemo) {
  if (t == NULL) return NULL;
  lsthunk_t* memo = subst_lookup(*pmemo, t);
  if (memo) return memo;

  switch (t->lt_type) {
  case LSTTYPE_REF: {
    lstref_target_t* target = t->lt_ref.ltr_target;
    // If the reference target hasn't been resolved yet, attempt a lazy
    // resolution against the captured environment so we can identify whether
    // this ref points to the current lambda parameter and capture it.
    if (!target && t->lt_ref.ltr_env) {
      target = lstenv_get(t->lt_ref.ltr_env, lsref_get_name(t->lt_ref.ltr_ref));
      if (target) t->lt_ref.ltr_target = target;
    }
    if (target) {
      // If this reference belongs to the same lambda parameter (including any subpattern
      // inside the parameter), capture the currently bound thunk for that specific ref.
      lstref_target_origin_t* org = target->lrt_origin;
      if (org && org->lrto_type == LSTRTYPE_LAMBDA && org->lrto_lambda.ltl_param == param) {
        lstpat_t* pref = target->lrt_pat; // the concrete ref node within the param pattern
        lsthunk_t* bound = pref ? lstpat_get_refbound(pref) : NULL;
        if (bound) return bound;
      }
    }
    return t;
  }
  case LSTTYPE_ALGE: {
    lssize_t n = t->lt_alge.lta_argc;
    lsthunk_t* nt = lsmalloc(lssizeof(lsthunk_t, lt_alge) + n * sizeof(lsthunk_t*));
    nt->lt_type = LSTTYPE_ALGE; nt->lt_whnf = nt;
    nt->lt_alge.lta_constr = t->lt_alge.lta_constr;
    nt->lt_alge.lta_argc = n;
    *pmemo = subst_bind(*pmemo, t, nt);
    for (lssize_t i = 0; i < n; i++)
      nt->lt_alge.lta_args[i] = lsthunk_subst_param_rec(t->lt_alge.lta_args[i], param, pmemo);
    return nt;
  }
  case LSTTYPE_APPL: {
    lssize_t n = t->lt_appl.lta_argc;
    lsthunk_t* nt = lsmalloc(lssizeof(lsthunk_t, lt_appl) + n * sizeof(lsthunk_t*));
    nt->lt_type = LSTTYPE_APPL; nt->lt_whnf = NULL;
    nt->lt_appl.lta_func = lsthunk_subst_param_rec(t->lt_appl.lta_func, param, pmemo);
    nt->lt_appl.lta_argc = n;
    *pmemo = subst_bind(*pmemo, t, nt);
    for (lssize_t i = 0; i < n; i++)
      nt->lt_appl.lta_args[i] = lsthunk_subst_param_rec(t->lt_appl.lta_args[i], param, pmemo);
    return nt;
  }
  case LSTTYPE_CHOICE: {
    lsthunk_t* nt = lsmalloc(sizeof(lsthunk_t));
  nt->lt_type = LSTTYPE_CHOICE; nt->lt_whnf = NULL;
    *pmemo = subst_bind(*pmemo, t, nt);
    nt->lt_choice.ltc_left = lsthunk_subst_param_rec(t->lt_choice.ltc_left, param, pmemo);
    nt->lt_choice.ltc_right = lsthunk_subst_param_rec(t->lt_choice.ltc_right, param, pmemo);
    return nt;
  }
  case LSTTYPE_LAMBDA: {
    // Capture references to the outer parameter even across lambda boundaries by
    // substituting inside the lambda body. Keep the parameter pattern as-is.
    lsthunk_t* nt = lsmalloc(sizeof(lsthunk_t));
    nt->lt_type = LSTTYPE_LAMBDA; nt->lt_whnf = nt;
    nt->lt_lambda.ltl_param = t->lt_lambda.ltl_param;
    *pmemo = subst_bind(*pmemo, t, nt);
    nt->lt_lambda.ltl_body = lsthunk_subst_param_rec(t->lt_lambda.ltl_body, param, pmemo);
    return nt;
  }
  case LSTTYPE_INT:
  case LSTTYPE_STR:
  case LSTTYPE_BUILTIN:
  default:
    return t;
  }
}

lsthunk_t* lsthunk_subst_param(lsthunk_t* thunk, lstpat_t* param) {
  subst_entry_t* memo = NULL;
  return lsthunk_subst_param_rec(thunk, param, &memo);
}
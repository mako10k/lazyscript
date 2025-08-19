#include "runtime/effects.h"
#include "runtime/unit.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "common/str.h"
#include "common/io.h"
#include "builtins/prelude.h"
#include "common/hash.h"
#include "builtins/ns.h"
#include "runtime/builtin.h"
// forward for namespace value constructor (not used now)
// lsthunk_t* lsbuiltin_ns_value(lssize_t argc, lsthunk_t* const* args, void* data);

// from to_string.c
lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data);

static lsthunk_t* lsbuiltin_prelude_exit(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: exit: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* val = lsthunk_eval0(args[0]);
  if (!val) return NULL;
  if (lsthunk_get_type(val) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: exit: invalid type\n");
    return NULL;
  }
  int is_zero = lsint_eq(lsthunk_get_int(val), lsint_new(0));
  exit(is_zero ? 0 : 1);
}

// Debug tracing toggle
#ifndef LS_TRACE
#define LS_TRACE 0
#endif

static lsthunk_t* lsbuiltin_prelude_println(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: println: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG println: begin\n");
  #endif
  lsthunk_t* thunk_str = lsthunk_eval0(args[0]);
  if (thunk_str == NULL)
    { 
      #if LS_TRACE
      lsprintf(stderr, 0, "DBG println: arg eval -> NULL\n");
      #endif
      return NULL;
    }
  #if LS_TRACE
  {
    const char* vt = "?";
    switch (lsthunk_get_type(thunk_str)) {
    case LSTTYPE_INT: vt = "int"; break; case LSTTYPE_STR: vt = "str"; break;
    case LSTTYPE_ALGE: vt = "alge"; break; case LSTTYPE_APPL: vt = "appl"; break;
    case LSTTYPE_LAMBDA: vt = "lambda"; break; case LSTTYPE_REF: vt = "ref"; break;
    case LSTTYPE_CHOICE: vt = "choice"; break; case LSTTYPE_BUILTIN: vt = "builtin"; break; }
    lsprintf(stderr, 0, "DBG println: arg type=%s\n", vt);
  }
  #endif
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR)
    thunk_str = lsbuiltin_to_string(1, args, NULL);
  #if LS_TRACE
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR) {
    lsprintf(stderr, 0, "DBG println: to_str did not return string\n");
  } else {
    lsprintf(stderr, 0, "DBG println: have string\n");
  }
  #endif
  const lsstr_t* str = lsthunk_get_str(thunk_str);
  lsstr_print_bare(stdout, LSPREC_LOWEST, 0, str);
  lsprintf(stdout, 0, "\n");
  return ls_make_unit();
}

static lsthunk_t* lsbuiltin_prelude_chain(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG chain: begin\n");
  #endif
  ls_effects_begin();
  lsthunk_t* action = lsthunk_eval0(args[0]);
  ls_effects_end();
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG chain: after action eval -> %s\n", action ? "ok" : "NULL");
  #endif
  if (!action) return NULL;
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG chain: apply cont\n");
  #endif
  return lsthunk_eval(cont, 1, &unit);
}

static lsthunk_t* lsbuiltin_prelude_return(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  return args[0];
}

static lsthunk_t* lsbuiltin_prelude_bind(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  // Evaluate the first computation to get its value (with effects enabled)
  ls_effects_begin();
  lsthunk_t* val = lsthunk_eval0(args[0]);
  ls_effects_end();
  if (!val) return NULL;
  // Apply the continuation to the value
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &val);
}

// 0-arity builtin getter for prelude.def
static lsthunk_t* lsbuiltin_getter0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; return (lsthunk_t*)data;
}

static lsthunk_t* lsbuiltin_prelude_def(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: def: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return NULL;
  lsthunk_t* namev = lsthunk_eval0(args[0]); if (!namev) return NULL;
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    lsprintf(stderr, 0, "E: def: expected a bare symbol as first argument\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  lsthunk_t* val = lsthunk_eval0(args[1]); if (!val) return NULL;
  lstenv_put_builtin(tenv, name, 0, lsbuiltin_getter0, val);
  return ls_make_unit();
}

lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);

static lsthunk_t* lsbuiltin_prelude_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  (void)argc;
  lsthunk_t* key = lsthunk_eval0(args[0]); if (!key) return NULL;
  if (lsthunk_get_type(key) != LSTTYPE_ALGE || lsthunk_get_argc(key) != 0) {
    lsprintf(stderr, 0, "E: prelude: expected a bare symbol (e.g., exit/println/chain/return)\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(key);
  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.exit"), 1, lsbuiltin_prelude_exit, NULL);
  if (lsstrcmp(name, lsstr_cstr("println")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.println"), 1, lsbuiltin_prelude_println, NULL);
  if (lsstrcmp(name, lsstr_cstr("def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, lsbuiltin_prelude_def, tenv);
  if (lsstrcmp(name, lsstr_cstr("require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, lsbuiltin_prelude_require, tenv);
  if (lsstrcmp(name, lsstr_cstr("chain")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.chain"), 2, lsbuiltin_prelude_chain, NULL);
  if (lsstrcmp(name, lsstr_cstr("bind")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.bind"), 2, lsbuiltin_prelude_bind, NULL);
  if (lsstrcmp(name, lsstr_cstr("return")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.return"), 1, lsbuiltin_prelude_return, NULL);
  if (lsstrcmp(name, lsstr_cstr("nsnew")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsnew"), 1, lsbuiltin_nsnew, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsdef")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdef"), 3, lsbuiltin_nsdef, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsnew0")) == 0)
    // Return the anonymous namespace value directly (0-arity application is awkward)
    return lsbuiltin_nsnew0(0, NULL, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsdefv")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdefv"), 3, lsbuiltin_nsdefv, tenv);
  lsprintf(stderr, 0, "E: prelude: unknown symbol: "); lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name); lsprintf(stderr, 0, "\n");
  return NULL;
}

// implemented in host (lazyscript.c)

// Registration helper used by host
void ls_register_builtin_prelude(lstenv_t* tenv) {
  lstenv_put_builtin(tenv, lsstr_cstr("prelude"), 1, lsbuiltin_prelude_dispatch, tenv);
}

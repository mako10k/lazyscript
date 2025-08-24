#include "runtime/effects.h"
#include "runtime/unit.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "common/str.h"
#include "common/io.h"
#include "builtins/prelude.h"
#include "common/hash.h"
#include "builtins/ns.h"
#include "builtins/builtin_loader.h"
#include "runtime/builtin.h"
#include "runtime/error.h"

// forward decls
lsthunk_t* lsbuiltin_prelude_eq(lssize_t argc, lsthunk_t* const* args, void* data);
#include <string.h>
#include <stdlib.h>
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
  lsthunk_t* val = ls_eval_arg(args[0], "exit: arg");
  if (lsthunk_is_err(val)) return val;
  if (!val) return ls_make_err("exit: arg eval");
  if (lsthunk_get_type(val) != LSTTYPE_INT) {
    return ls_make_err("exit: invalid type");
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
  lsthunk_t* thunk_str = ls_eval_arg(args[0], "println: arg");
  if (lsthunk_is_err(thunk_str)) return thunk_str;
  if (thunk_str == NULL)
    {
      #if LS_TRACE
      lsprintf(stderr, 0, "DBG println: arg eval -> NULL\n");
      #endif
      return ls_make_err("println: arg eval");
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
  if (lsthunk_is_err(thunk_str)) return thunk_str;
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
  lsthunk_t* action = ls_eval_arg(args[0], "chain: action");
  ls_effects_end();
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG chain: after action eval -> %s\n", action ? "ok" : "NULL");
  #endif
  if (lsthunk_is_err(action)) return action;
  if (!action) return ls_make_err("chain: action eval");
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
  lsthunk_t* val = ls_eval_arg(args[0], "bind: value");
  ls_effects_end();
  if (lsthunk_is_err(val)) return val;
  if (!val) return ls_make_err("bind: value eval");
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
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return ls_make_err("def: no env");
  lsthunk_t* namev = ls_eval_arg(args[0], "def: name"); if (lsthunk_is_err(namev)) return namev; if (!namev) return ls_make_err("def: name eval");
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    return ls_make_err("def: expected bare symbol");
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  lsthunk_t* val = ls_eval_arg(args[1], "def: value"); if (lsthunk_is_err(val)) return val; if (!val) return ls_make_err("def: value eval");
  lstenv_put_builtin(tenv, name, 0, lsbuiltin_getter0, val);
  return ls_make_unit();
}

lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_prelude_include(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_prelude_ns_self(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* lsbuiltin_prelude_import(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* lsbuiltin_prelude_with_import(lssize_t argc, lsthunk_t* const* args, void* data);

// (~internal .key) dispatch for environment-affecting ops (Prelude evaluation scope only)
lsthunk_t* lsbuiltin_prelude_internal_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data; (void)argc;
  lsthunk_t* keyv = ls_eval_arg(args[0], "internal: key"); if (lsthunk_is_err(keyv)) return keyv; if (!keyv) return ls_make_err("internal: key eval");
  if (lsthunk_get_type(keyv) != LSTTYPE_SYMBOL) {
    return ls_make_err("internal: expected symbol like .require");
  }
  const lsstr_t* s = lsthunk_get_symbol(keyv);
  if (lsstrcmp(s, lsstr_cstr(".require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, lsbuiltin_prelude_require, tenv);
  if (lsstrcmp(s, lsstr_cstr(".requirePure")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.requirePure"), 1, lsbuiltin_prelude_require_pure, tenv);
  if (lsstrcmp(s, lsstr_cstr(".include")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.include"), 1, lsbuiltin_prelude_require_pure, tenv);
  if (lsstrcmp(s, lsstr_cstr(".import")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.import"), 1, lsbuiltin_prelude_import, tenv);
  if (lsstrcmp(s, lsstr_cstr(".withImport")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.withImport"), 2, lsbuiltin_prelude_with_import, tenv);
  if (lsstrcmp(s, lsstr_cstr(".def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, lsbuiltin_prelude_def, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsnew")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsnew"), 1, lsbuiltin_nsnew, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsdef")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdef"), 3, lsbuiltin_nsdef, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsnew0")) == 0) {
    if (!ls_effects_allowed()) { lsprintf(stderr, 0, "E: nsnew0: effect used in pure context (enable seq/chain)\n"); return NULL; }
    return lsbuiltin_nsnew0(0, NULL, tenv);
  }
  if (lsstrcmp(s, lsstr_cstr(".nsdefv")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdefv"), 3, lsbuiltin_nsdefv, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsMembers")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsMembers"), 1, lsbuiltin_ns_members, NULL);
  if (lsstrcmp(s, lsstr_cstr(".nsSelf")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsSelf"), 0, lsbuiltin_prelude_ns_self, NULL);
  if (lsstrcmp(s, lsstr_cstr(".builtin")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.builtin"), 1, lsbuiltin_prelude_builtin, tenv);
  return ls_make_err("internal: unknown key");
}

// prelude.import: import members of a namespace value into current env
static void ls_import_bind_cb(const lsstr_t* key, lshash_data_t value, void* data) {
  if (!key) return;
  // Keys are encoded as 'S' + ".name"; strip tag and leading '.'
  const char* bytes = lsstr_get_buf(key);
  lssize_t    len   = lsstr_get_len(key);
  if (!bytes || len <= 1) return;
  const char* sym   = bytes + 1; // drop 'S'
  lssize_t    slen  = len - 1;
  if (slen > 0 && sym[0] == '.') { sym++; slen--; }
  if (slen <= 0) return;
  struct { lstenv_t* tenv; } *ctx = (void*)data;
  const lsstr_t* vname = lsstr_new(sym, slen);
  lsthunk_t*     val   = (lsthunk_t*)value;
  // define as 0-arity getter bound to the value thunk
  extern lsthunk_t* lsbuiltin_getter0(lssize_t, lsthunk_t* const*, void*);
  lstenv_put_builtin(ctx->tenv, vname, 0, lsbuiltin_getter0, val);
}

static void prelude_import_cb(const lsstr_t* sym, lsthunk_t* value, void* data) {
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return;
  // sym comes without tag; may start with '.'
  const char* s = lsstr_get_buf(sym); lssize_t n = lsstr_get_len(sym);
  if (!s || n <= 0) return;
  const lsstr_t* vname = lsstr_new(s, n);
  lstenv_put_builtin(tenv, vname, 0, lsbuiltin_getter0, value);
}

static lsthunk_t* lsbuiltin_prelude_import(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: import: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return ls_make_err("import: no env");
  lsthunk_t* nsv = ls_eval_arg(args[0], "import: ns");
  if (lsthunk_is_err(nsv)) return nsv;
  if (!nsv) return ls_make_err("import: ns eval");
  if (!lsns_foreach_member(nsv, prelude_import_cb, tenv)) return ls_make_err("import: invalid namespace");
  return ls_make_unit();
}

static lsthunk_t* lsbuiltin_prelude_with_import(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: withImport: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return ls_make_err("withImport: no env");
  lsthunk_t* nsv = ls_eval_arg(args[0], "withImport: ns");
  if (lsthunk_is_err(nsv)) return nsv;
  if (!nsv) return ls_make_err("withImport: ns eval");
  if (!lsns_foreach_member(nsv, prelude_import_cb, tenv)) return ls_make_err("withImport: invalid namespace");
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

static lsthunk_t* lsbuiltin_prelude_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  (void)argc;
  lsthunk_t* key = ls_eval_arg(args[0], "prelude: key"); if (lsthunk_is_err(key)) return key; if (!key) return ls_make_err("prelude: key eval");
  if (lsthunk_get_type(key) != LSTTYPE_ALGE || lsthunk_get_argc(key) != 0) {
    return ls_make_err("prelude: expected bare symbol");
  }
  const lsstr_t* name = lsthunk_get_constr(key);
  // Special ephemeral binding: ~builtins is available while evaluating Prelude.ls
  // It points to the loaded core builtins namespace via builtin loader.
  // We install it lazily on first access to any prelude symbol.
  static int prelude_builtins_bound = 0;
  if (!prelude_builtins_bound && tenv) {
    // (~prelude builtin) "core" -> namespace value
    lsthunk_t* arg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* argv1[1] = { arg };
    lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, argv1, tenv);
    if (core_ns) {
      // Bind as 0-arity getter under name "builtins" (source uses ~builtins)
      extern lsthunk_t* lsbuiltin_getter0(lssize_t, lsthunk_t* const*, void*);
      lstenv_put_builtin(tenv, lsstr_cstr("builtins"), 0, lsbuiltin_getter0, core_ns);
  // Also bind ~internal namespace for env-affecting ops
  lsthunk_t* internal_ns = lsthunk_new_builtin(lsstr_cstr("prelude.internal"), 1, lsbuiltin_prelude_internal_dispatch, tenv);
  lstenv_put_builtin(tenv, lsstr_cstr("internal"), 0, lsbuiltin_getter0, internal_ns);
  prelude_builtins_bound = 1;
    }
  }
  // eq: forward to core builtins module's .eq
  if (lsstrcmp(name, lsstr_cstr("eq")) == 0) {
    // Load core namespace and return its .eq member
    lsthunk_t* carg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* cargv[1] = { carg };
    lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, cargv, tenv);
    if (!core_ns) return ls_make_err("prelude.eq: no core builtins");
    lsthunk_t* key = lsthunk_new_symbol(lsstr_cstr(".eq"));
    lsthunk_t* argv1[1] = { key };
    return lsthunk_eval(core_ns, 1, argv1);
  }
  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.exit"), 1, lsbuiltin_prelude_exit, NULL);
  if (lsstrcmp(name, lsstr_cstr("println")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.println"), 1, lsbuiltin_prelude_println, NULL);
  if (lsstrcmp(name, lsstr_cstr("def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, lsbuiltin_prelude_def, tenv);
  if (lsstrcmp(name, lsstr_cstr("require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, lsbuiltin_prelude_require, tenv);
  if (lsstrcmp(name, lsstr_cstr("include")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.include"), 1, lsbuiltin_prelude_require_pure, tenv);
  if (lsstrcmp(name, lsstr_cstr("import")) == 0 || lsstrcmp(name, lsstr_cstr(".import")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.import"), 1, lsbuiltin_prelude_import, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsSelf")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsSelf"), 0, lsbuiltin_prelude_ns_self, NULL);
  if (lsstrcmp(name, lsstr_cstr("withImport")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.withImport"), 2, lsbuiltin_prelude_with_import, tenv);
  if (lsstrcmp(name, lsstr_cstr("chain")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.chain"), 2, lsbuiltin_prelude_chain, NULL);
  if (lsstrcmp(name, lsstr_cstr("bind")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.bind"), 2, lsbuiltin_prelude_bind, NULL);
  if (lsstrcmp(name, lsstr_cstr("return")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.return"), 1, lsbuiltin_prelude_return, NULL);
  if (lsstrcmp(name, lsstr_cstr("lt")) == 0) {
    lsthunk_t* carg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* cargv[1] = { carg };
    lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, cargv, tenv);
    if (!core_ns) return ls_make_err("prelude.lt: no core builtins");
    lsthunk_t* key = lsthunk_new_symbol(lsstr_cstr(".lt"));
    lsthunk_t* argv1[1] = { key };
    return lsthunk_eval(core_ns, 1, argv1);
  }
  if (lsstrcmp(name, lsstr_cstr("nsnew")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsnew"), 1, lsbuiltin_nsnew, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsdef")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdef"), 3, lsbuiltin_nsdef, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsnew0")) == 0) {
    // Direct creation is effectful; guard under strict-effects
    if (!ls_effects_allowed()) {
      lsprintf(stderr, 0, "E: nsnew0: effect used in pure context (enable seq/chain)\n");
      return NULL;
    }
    // Return the anonymous namespace value directly (0-arity application is awkward)
    return lsbuiltin_nsnew0(0, NULL, tenv);
  }
  if (lsstrcmp(name, lsstr_cstr("nsdefv")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdefv"), 3, lsbuiltin_nsdefv, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsMembers")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsMembers"), 1, lsbuiltin_ns_members, NULL);
  // builtins: deprecated facade; prefer ~builtins via Prelude.ls
  if (lsstrcmp(name, lsstr_cstr("builtins")) == 0) {
    extern lsthunk_t* lsbuiltin_prelude_builtin(lssize_t, lsthunk_t* const*, void*);
    lsthunk_t* arg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* argv[1] = { arg };
    return lsbuiltin_prelude_builtin(1, argv, tenv);
  }
  // builtin loader (supports both builtin and .builtin keys)
  if (lsstrcmp(name, lsstr_cstr("builtin")) == 0 || lsstrcmp(name, lsstr_cstr(".builtin")) == 0) {
    extern lsthunk_t* lsbuiltin_prelude_builtin(lssize_t, lsthunk_t* const*, void*);
    return lsthunk_new_builtin(lsstr_cstr("prelude.builtin"), 1, lsbuiltin_prelude_builtin, tenv);
  }
  // nslit$N dispatch for pure namespace literal
  const char* cname = lsstr_get_buf(name);
  if (strncmp(cname, "nslit$", 6) == 0) {
    long n = strtol(cname + 6, NULL, 10);
    return lsthunk_new_builtin(lsstr_cstr("prelude.nslit"), n, lsbuiltin_nslit, NULL);
  }
  lsprintf(stderr, 0, "E: prelude: unknown symbol: ");
  lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
  lsprintf(stderr, 0, "\n");
  // Returning NULL here would lead to segmentation faults when callers
  // attempt to use the missing value.  Instead propagate a runtime
  // error so evaluation can fail gracefully.
  return ls_make_err("prelude: unknown symbol");
}

// implemented in host (lazyscript.c)

// Registration helper used by host
void ls_register_builtin_prelude(lstenv_t* tenv) {
  lstenv_put_builtin(tenv, lsstr_cstr("prelude"), 1, lsbuiltin_prelude_dispatch, tenv);
  // Stable alias for built-in prelude dispatch (used by parser sugar)
  lstenv_put_builtin(tenv, lsstr_cstr("prelude$builtin"), 1, lsbuiltin_prelude_dispatch, tenv);
}

// Implementation of prelude.eq (2-arity)
lsthunk_t* lsbuiltin_prelude_eq(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  lsthunk_t* a = ls_eval_arg(args[0], "eq: arg1"); if (lsthunk_is_err(a)) return a; if (!a) return ls_make_err("eq: arg1 eval");
  lsthunk_t* b = ls_eval_arg(args[1], "eq: arg2"); if (lsthunk_is_err(b)) return b; if (!b) return ls_make_err("eq: arg2 eval");
  // int equality
  if (lsthunk_get_type(a) == LSTTYPE_INT && lsthunk_get_type(b) == LSTTYPE_INT) {
    int eq = lsint_eq(lsthunk_get_int(a), lsthunk_get_int(b));
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  // symbol equality (explicit symbol type)
  if (lsthunk_get_type(a) == LSTTYPE_SYMBOL && lsthunk_get_type(b) == LSTTYPE_SYMBOL) {
    const lsstr_t* sa = lsthunk_get_symbol(a);
    const lsstr_t* sb = lsthunk_get_symbol(b);
    int eq = (lsstrcmp(sa, sb) == 0);
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  // algebraic 0-arity constructors (used for .a style symbols in lists)
  if (lsthunk_get_type(a) == LSTTYPE_ALGE && lsthunk_get_argc(a) == 0 &&
      lsthunk_get_type(b) == LSTTYPE_ALGE && lsthunk_get_argc(b) == 0) {
    const lsstr_t* ca = lsthunk_get_constr(a);
    const lsstr_t* cb = lsthunk_get_constr(b);
    int eq = (lsstrcmp(ca, cb) == 0);
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("false"), 0, NULL), NULL);
}

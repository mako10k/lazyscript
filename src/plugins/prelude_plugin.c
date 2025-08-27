// Clean implementation of prelude plugin with env-gated trace helpers
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "expr/ealge.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/effects.h"
#include "runtime/builtin.h"
#include "builtins/ns.h"
#include "builtins/builtin_loader.h"
#include <stdlib.h>
#include <string.h>
#include <gc.h>
#include <assert.h>
#include "runtime/error.h"
#include "runtime/trace.h"
#include "runtime/unit.h"

// forward decls used by pl_internal_dispatch
static lsthunk_t* pl_require(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_include(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_import(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_importOpt(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_withImport(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_def(lssize_t argc, lsthunk_t* const* args, void* data);
// forward for callback
static void pl_import_cb(const lsstr_t* sym, lsthunk_t* value, void* data);

static lsthunk_t* pl_getter0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; return (lsthunk_t*)data;
}

static lsthunk_t* pl_def(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  assert(argc == 2);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: def: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv) {
    return NULL;
  }
  lsthunk_t* namev = lsthunk_eval0(args[0]);
  if (namev == NULL)
    return NULL;
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    lsprintf(stderr, 0, "E: def: expected bare symbol\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  lsthunk_t* val = lsthunk_eval0(args[1]);
  if (val == NULL)
    return NULL;
  lstenv_put_builtin(tenv, name, 0, pl_getter0, val);
  return ls_make_unit();
}

static lsthunk_t* pl_exit(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: exit: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* val = lsthunk_eval0(args[0]);
  if (val == NULL)
    return NULL;
  if (lsthunk_get_type(val) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: exit: invalid type\n");
    return NULL;
  }
  int is_zero = lsint_eq(lsthunk_get_int(val), lsint_new(0));
  exit(is_zero ? 0 : 1);
}

// Optional gating for debug helpers via env var
static int pl_trace_enabled(void) {
  static int init = 0;
  static int enabled = 0;
  if (!init) {
    const char* v = getenv("LAZYSCRIPT_ENABLE_TRACE");
    enabled = (v && v[0] && v[0] != '0') ? 1 : 0;
    init = 1;
  }
  return enabled;
}

// trace: debug helper that prints source location, msg, and a shallow view of expr to stderr
// Signature: trace msg expr = expr (does not evaluate expr)
static lsthunk_t* pl_trace(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* expr = args[1];
  if (!pl_trace_enabled()) {
    return expr; // disabled: no-op
  }
  // Prepare message: evaluate first arg to WHNF to allow strings/symbols/etc.
  lsthunk_t* msgv = lsthunk_eval0(args[0]);
  // Obtain current source location from trace system
  int tid = lstrace_current();
  if (tid >= 0) {
    lstrace_span_t s = lstrace_lookup(tid);
    lsprintf(stderr, 0, "[trace] %s:%d:%d-: ", s.filename ? s.filename : "<unknown>", s.first_line, s.first_column);
  } else {
    lsprintf(stderr, 0, "[trace] <no-loc>: ");
  }
  // Print message
  if (msgv && lsthunk_get_type(msgv) == LSTTYPE_STR) {
    const lsstr_t* str = lsthunk_get_str(msgv);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, str);
  } else if (msgv) {
    lsthunk_dprint(stderr, LSPREC_LOWEST, 0, msgv);
  } else {
    lsprintf(stderr, 0, "<msg-eval-null>");
  }
  // Print separator and shallow expr (no evaluation)
  lsprintf(stderr, 0, " :: ");
  if (expr) lsthunk_dprint(stderr, LSPREC_LOWEST, 0, expr);
  lsprintf(stderr, 0, "\n");
  return expr;
}

// force: evaluate to WHNF and return the evaluated thunk
static lsthunk_t* pl_force(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; assert(argc == 1); assert(args != NULL);
  if (!pl_trace_enabled()) {
    return args[0]; // disabled: no-op
  }
  lsthunk_t* v = lsthunk_eval0(args[0]);
  return v ? v : NULL;
}

// traceForce: like trace but forces expr to WHNF before printing; returns expr (evaluated)
static lsthunk_t* pl_trace_force(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; assert(argc == 2); assert(args != NULL);
  lsthunk_t* expr = args[1];
  if (!pl_trace_enabled()) {
    return expr; // disabled: no-op
  }
  lsthunk_t* msgv = lsthunk_eval0(args[0]);
  lsthunk_t* val = lsthunk_eval0(expr);
  int tid = lstrace_current();
  if (tid >= 0) {
    lstrace_span_t s = lstrace_lookup(tid);
    lsprintf(stderr, 0, "[trace!] %s:%d:%d-: ", s.filename ? s.filename : "<unknown>", s.first_line, s.first_column);
  } else {
    lsprintf(stderr, 0, "[trace!] <no-loc>: ");
  }
  if (msgv && lsthunk_get_type(msgv) == LSTTYPE_STR) {
    const lsstr_t* str = lsthunk_get_str(msgv);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, str);
  } else if (msgv) {
    lsthunk_dprint(stderr, LSPREC_LOWEST, 0, msgv);
  } else {
    lsprintf(stderr, 0, "<msg-eval-null>");
  }
  lsprintf(stderr, 0, " :: ");
  if (val) lsthunk_dprint(stderr, LSPREC_LOWEST, 0, val); else lsprintf(stderr, 0, "<eval-null>");
  lsprintf(stderr, 0, "\n");
  return val ? val : NULL;
}

// Forward to host implementations for module loading
extern lsthunk_t* lsbuiltin_prelude_require(lssize_t, lsthunk_t* const*, void*);
extern lsthunk_t* lsbuiltin_prelude_require_opt(lssize_t, lsthunk_t* const*, void*);
extern lsthunk_t* lsbuiltin_prelude_include(lssize_t, lsthunk_t* const*, void*);

static lsthunk_t* pl_require(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  return lsbuiltin_prelude_require(argc, args, tenv);
}

static lsthunk_t* pl_include(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  return lsbuiltin_prelude_include(argc, args, tenv);
}

// Option-wrapped import: returns Some () on success, None on invalid namespace
static lsthunk_t* pl_importOpt(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: importOpt: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv) return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]); if (nsv == NULL) return NULL;
  if (!lsns_foreach_member(nsv, pl_import_cb, tenv))
    return lsthunk_new_ealge(lsealge_new(lsstr_cstr("None"), 0, NULL), NULL);
  const lsexpr_t* args1[1] = { lsexpr_new_alge(lsealge_new(lsstr_cstr("()"), 0, NULL)) };
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("Some"), 1, args1), NULL);
}

static lsthunk_t* pl_chain(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  ls_effects_begin();
  lsthunk_t* action = ls_eval_arg(args[0], "chain: action");
  ls_effects_end();
  if (lsthunk_is_err(action))
    return action;
  if (!action)
    return ls_make_err("chain: action eval");
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

static lsthunk_t* pl_bind(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  ls_effects_begin();
  lsthunk_t* val = ls_eval_arg(args[0], "bind: value");
  ls_effects_end();
  if (lsthunk_is_err(val))
    return val;
  if (!val)
    return ls_make_err("bind: value eval");
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &val);
}

static lsthunk_t* pl_return(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  return args[0];
}

// eq: simple equality for common types (int, symbol/algebraic 0-arity)
// Note: equality is provided by core builtins and forwarded via prelude.dispatch("eq").

// import/withImport for plugin prelude
static void pl_import_cb(const lsstr_t* sym, lsthunk_t* value, void* data) {
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return;
  lstenv_put_builtin(tenv, sym, 0, pl_getter0, value);
}

static lsthunk_t* pl_import(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: import: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv) return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]); if (nsv == NULL) return NULL;
  if (!lsns_foreach_member(nsv, pl_import_cb, tenv)) return ls_make_err("import: invalid namespace");
  return ls_make_unit();
}

static lsthunk_t* pl_withImport(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: withImport: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv) return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]); if (nsv == NULL) return NULL;
  if (!lsns_foreach_member(nsv, pl_import_cb, tenv)) return ls_make_err("withImport: invalid namespace");
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

static lsthunk_t* pl_plugin_hello(void) {
  return lsthunk_new_str(lsstr_cstr("plugin"));
}

// main 1-arg dispatcher for prelude: maps names to builtins
static lsthunk_t* pl_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_t* namev = lsthunk_eval0(args[0]); if (!namev) return NULL;
  const lsstr_t* name = NULL;
  if (lsthunk_get_type(namev) == LSTTYPE_ALGE && lsthunk_get_argc(namev) == 0) name = lsthunk_get_constr(namev);
  else if (lsthunk_get_type(namev) == LSTTYPE_SYMBOL) name = lsthunk_get_symbol(namev);
  else return ls_make_err("prelude: expected name symbol");

  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.exit"), 1, pl_exit, NULL);
  if (lsstrcmp(name, lsstr_cstr("def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, pl_def, tenv);
  if (lsstrcmp(name, lsstr_cstr("require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, pl_require, tenv);
  if (lsstrcmp(name, lsstr_cstr("requireOpt")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.requireOpt"), 1, lsbuiltin_prelude_require_opt, tenv);
  if (lsstrcmp(name, lsstr_cstr("include")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.include"), 1, pl_include, tenv);
  if (lsstrcmp(name, lsstr_cstr("import")) == 0 || lsstrcmp(name, lsstr_cstr(".import")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.import"), 1, pl_import, tenv);
  if (lsstrcmp(name, lsstr_cstr("importOpt")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.importOpt"), 1, pl_importOpt, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsSelf")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsSelf"), 0, lsbuiltin_prelude_ns_self, NULL);
  if (lsstrcmp(name, lsstr_cstr("withImport")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.withImport"), 2, pl_withImport, tenv);
  if (lsstrcmp(name, lsstr_cstr("chain")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.chain"), 2, pl_chain, NULL);
  if (lsstrcmp(name, lsstr_cstr("bind")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.bind"), 2, pl_bind, NULL);
  if (lsstrcmp(name, lsstr_cstr("return")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.return"), 1, pl_return, NULL);
  if (lsstrcmp(name, lsstr_cstr("trace")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.trace"), 2, pl_trace, NULL);
  if (lsstrcmp(name, lsstr_cstr("force")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.force"), 1, pl_force, NULL);
  if (lsstrcmp(name, lsstr_cstr("traceForce")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.traceForce"), 2, pl_trace_force, NULL);
  if (lsstrcmp(name, lsstr_cstr("lt")) == 0) {
    lsthunk_t* carg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* cargv[1] = { carg };
    lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, cargv, tenv);
    if (!core_ns) return ls_make_err("prelude.lt: no core builtins");
    lsthunk_t* key = lsthunk_new_symbol(lsstr_cstr(".lt"));
    lsthunk_t* argv1[1] = { key };
    return lsthunk_eval(core_ns, 1, argv1);
  }
  if (lsstrcmp(name, lsstr_cstr("eq")) == 0) {
    lsthunk_t* carg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* cargv[1] = { carg };
    lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, cargv, tenv);
    if (!core_ns) return ls_make_err("prelude.eq: no core builtins");
    lsthunk_t* key = lsthunk_new_symbol(lsstr_cstr(".eq"));
    lsthunk_t* argv1[1] = { key };
    return lsthunk_eval(core_ns, 1, argv1);
  }
  if (lsstrcmp(name, lsstr_cstr("nsMembers")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsMembers"), 1, lsbuiltin_ns_members, NULL);
  if (lsstrcmp(name, lsstr_cstr("builtin")) == 0 || lsstrcmp(name, lsstr_cstr(".builtin")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.builtin"), 1, lsbuiltin_prelude_builtin, tenv);
  const char* cname = lsstr_get_buf(name);
  if (strncmp(cname, "nslit$", 6) == 0) {
    long n = strtol(cname + 6, NULL, 10);
    return lsthunk_new_builtin(lsstr_cstr("prelude.nslit"), n, lsbuiltin_nslit, NULL);
  }
  if (lsstrcmp(name, lsstr_cstr("pluginHello")) == 0)
    return pl_plugin_hello();
  lsprintf(stderr, 0, "E: prelude: unknown symbol: ");
  lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
  lsprintf(stderr, 0, "\n");
  return ls_make_err("prelude: unknown symbol");
}

int ls_prelude_register(lstenv_t* tenv) {
  if (!tenv)
    return -1;
  lstenv_put_builtin(tenv, lsstr_cstr("prelude"), 1, pl_dispatch, tenv);
  // Stable alias kept as builtin even after prelude is rebound to a value
  lstenv_put_builtin(tenv, lsstr_cstr("prelude$builtin"), 1, pl_dispatch, tenv);
  return 0;
}

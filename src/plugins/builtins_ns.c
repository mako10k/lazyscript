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
#include "plugins/plugin_common.h"
#include "runtime/unit.h"

// forward decls used by pl_internal_dispatch
static lsthunk_t* pl_require(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_include(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_import(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_importOpt(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_withImport(lssize_t argc, lsthunk_t* const* args, void* data);
// forward for callback
static void pl_import_cb(const lsstr_t* sym, lsthunk_t* value, void* data);

// Core runtime builtins forwarded under prelude
extern lsthunk_t* lsbuiltin_print(lssize_t argc, lsthunk_t* const* args, void* data);
extern lsthunk_t* lsbuiltin_seq(lssize_t argc, lsthunk_t* const* args, void* data);
extern lsthunk_t* lsbuiltin_add(lssize_t argc, lsthunk_t* const* args, void* data);
extern lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data);
extern lsthunk_t* lsbuiltin_prelude_require(lssize_t, lsthunk_t* const*, void*);
extern lsthunk_t* lsbuiltin_prelude_include(lssize_t, lsthunk_t* const*, void*);

// println: print followed by newline; returns unit
static lsthunk_t* pl_println(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)data;
  lsthunk_t* r = lsbuiltin_print(1, args, NULL);
  if (!r)
    return NULL;
  lsprintf(stdout, 0, "\n");
  return ls_make_unit();
}

static lsthunk_t* pl_getter0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)args;
  return (lsthunk_t*)data;
}

// require/include wrappers delegating to host builtins
static lsthunk_t* pl_require(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  return lsbuiltin_prelude_require(argc, args, tenv);
}

static lsthunk_t* pl_include(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  return lsbuiltin_prelude_include(argc, args, tenv);
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
  static int init    = 0;
  static int enabled = 0;
  if (!init) {
    const char* v = getenv("LAZYSCRIPT_ENABLE_TRACE");
    enabled       = (v && v[0] && v[0] != '0') ? 1 : 0;
    init          = 1;
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
  plugin_trace_prefix("trace");
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
  if (expr)
    lsthunk_dprint(stderr, LSPREC_LOWEST, 0, expr);
  lsprintf(stderr, 0, "\n");
  return expr;
}

// force: evaluate to WHNF and return the evaluated thunk
static lsthunk_t* pl_force(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  if (!pl_trace_enabled()) {
    return args[0]; // disabled: no-op
  }
  lsthunk_t* v = lsthunk_eval0(args[0]);
  return v ? v : NULL;
}

// traceForce: like trace but forces expr to WHNF before printing; returns expr (evaluated)
static lsthunk_t* pl_trace_force(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* expr = args[1];
  if (!pl_trace_enabled()) {
    return expr; // disabled: no-op (do not force)
  }
  // Evaluate message and expr
  lsthunk_t* msgv = lsthunk_eval0(args[0]);
  lsthunk_t* val  = lsthunk_eval0(expr);
  plugin_trace_prefix("traceForce");
  if (msgv && lsthunk_get_type(msgv) == LSTTYPE_STR) {
    const lsstr_t* str = lsthunk_get_str(msgv);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, str);
  } else if (msgv) {
    lsthunk_dprint(stderr, LSPREC_LOWEST, 0, msgv);
  } else {
    lsprintf(stderr, 0, "<msg-eval-null>");
  }
  lsprintf(stderr, 0, " :: ");
  if (val)
    lsthunk_dprint(stderr, LSPREC_LOWEST, 0, val);
  lsprintf(stderr, 0, "\n");
  return val ? val : NULL;
}

// requireOpt: try require; return Some () on success, None on failure. Effectful.
static lsthunk_t* pl_require_opt(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: requireOpt: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv)
    return ls_make_err("requireOpt: no env");
  // Delegate to require; map error -> None, success -> Some ()
  lsthunk_t* ret = lsbuiltin_prelude_require(1, args, tenv);
  if (ret == NULL)
    return NULL;
  if (lsthunk_is_err(ret)) {
    // None
    return lsthunk_new_ealge(lsealge_new(lsstr_cstr("None"), 0, NULL), NULL);
  }
  // Some ()
  const lsexpr_t* some_arg     = lsexpr_new_alge(lsealge_new(lsstr_cstr("()"), 0, NULL));
  const lsexpr_t* some_args[1] = { some_arg };
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("Some"), 1, some_args), NULL);
}

// strcat: pure string concatenation
static lsthunk_t* pl_strcat(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)data;
  lsthunk_t* a = lsthunk_eval0(args[0]);
  if (!a)
    return NULL;
  lsthunk_t* b = lsthunk_eval0(args[1]);
  if (!b)
    return NULL;
  if (lsthunk_get_type(a) != LSTTYPE_STR || lsthunk_get_type(b) != LSTTYPE_STR)
    return ls_make_err("strcat: expected string operands");
  size_t len = 0;
  char*  buf = NULL;
  FILE*  fp  = lsopen_memstream_gc(&buf, &len);
  lsstr_print_bare(fp, LSPREC_LOWEST, 0, lsthunk_get_str(a));
  lsstr_print_bare(fp, LSPREC_LOWEST, 0, lsthunk_get_str(b));
  fclose(fp);
  return lsthunk_new_str(lsstr_new(buf, (lssize_t)len));
}

// Option-like import: returns true on success, false on invalid namespace
static lsthunk_t* pl_importOpt(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: importOpt: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv)
    return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]);
  if (nsv == NULL)
    return NULL;
  if (!lsns_foreach_member(nsv, pl_import_cb, tenv))
    return lsthunk_new_ealge(lsealge_new(lsstr_cstr("false"), 0, NULL), NULL);
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("true"), 0, NULL), NULL);
}

// (~prelude .env key) dispatcher to access env-scoped operations
static lsthunk_t* pl_dispatch_env(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  (void)argc;
  if (!tenv)
    return ls_make_err("prelude.env: no env");
  lsthunk_t* keyv = lsthunk_eval0(args[0]);
  if (!keyv)
    return NULL;
  if (lsthunk_get_type(keyv) != LSTTYPE_SYMBOL)
    return ls_make_err("prelude.env: expected symbol");
  const lsstr_t* s = lsthunk_get_symbol(keyv);
  if (lsstrcmp(s, lsstr_cstr(".require")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.require"), 1, pl_require, tenv,
                                    LSBATTR_EFFECT | LSBATTR_ENV_READ);
  if (lsstrcmp(s, lsstr_cstr(".include")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.include"), 1, pl_include, tenv,
                                    LSBATTR_ENV_READ);
  if (lsstrcmp(s, lsstr_cstr(".print")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.print"), 1, lsbuiltin_print, NULL,
                                    LSBATTR_EFFECT);
  if (lsstrcmp(s, lsstr_cstr(".println")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.println"), 1, pl_println, NULL,
                                    LSBATTR_EFFECT);
  if (lsstrcmp(s, lsstr_cstr(".exit")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.exit"), 1, pl_exit, NULL, LSBATTR_EFFECT);
  if (lsstrcmp(s, lsstr_cstr(".import")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.import"), 1, pl_import, tenv,
                                    LSBATTR_ENV_WRITE | LSBATTR_EFFECT);
  if (lsstrcmp(s, lsstr_cstr(".importOpt")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.importOpt"), 1, pl_importOpt, tenv,
                                    LSBATTR_ENV_WRITE | LSBATTR_EFFECT);
  if (lsstrcmp(s, lsstr_cstr(".requireOpt")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.requireOpt"), 1, pl_require_opt, tenv,
                                    LSBATTR_EFFECT | LSBATTR_ENV_READ);
  if (lsstrcmp(s, lsstr_cstr(".withImport")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.withImport"), 2, pl_withImport, tenv,
                                    LSBATTR_ENV_WRITE | LSBATTR_EFFECT);
  /* .def removed */
  if (lsstrcmp(s, lsstr_cstr(".builtin")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.builtin"), 1, lsbuiltin_prelude_builtin,
                                    tenv, LSBATTR_EFFECT | LSBATTR_ENV_READ);
  /* .raise removed: caret ^(expr) now has a dedicated AST/runtime */
  return ls_make_err("prelude.env: unknown key");
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
  lstenv_t* tenv = (lstenv_t*)data;
  if (!tenv)
    return;
  // Bind original key as-is (may start with '.')
  lstenv_put_builtin(tenv, sym, 0, pl_getter0, value);
  // If symbol starts with '.', also bind an alias without the leading dot
  const char* s = lsstr_get_buf(sym);
  lssize_t    n = lsstr_get_len(sym);
  if (s && n > 1 && s[0] == '.') {
    const lsstr_t* alias = lsstr_new(s + 1, n - 1);
    lstenv_put_builtin(tenv, alias, 0, pl_getter0, value);
  }
}

static lsthunk_t* pl_import(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: import: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv)
    return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]);
  if (nsv == NULL)
    return NULL;
  if (!lsns_foreach_member(nsv, pl_import_cb, tenv))
    return ls_make_err("import: invalid namespace");
  return ls_make_unit();
}

static lsthunk_t* pl_withImport(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: withImport: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv)
    return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]);
  if (nsv == NULL)
    return NULL;
  if (!lsns_foreach_member(nsv, pl_import_cb, tenv))
    return ls_make_err("withImport: invalid namespace");
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

// pluginHello removed
// nsdef stub removed

// main 1-arg dispatcher for prelude: maps names to builtins
static lsthunk_t* pl_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_t* namev = lsthunk_eval0(args[0]);
  if (!namev)
    return NULL;
  const lsstr_t* name = NULL;
  if (lsthunk_get_type(namev) == LSTTYPE_ALGE && lsthunk_get_argc(namev) == 0)
    name = lsthunk_get_constr(namev);
  else if (lsthunk_get_type(namev) == LSTTYPE_SYMBOL)
    name = lsthunk_get_symbol(namev);
  else
    return ls_make_err("prelude: expected name symbol");
  if (lsstrcmp(name, lsstr_cstr(".env")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.env"), 1, pl_dispatch_env, tenv,
                                    LSBATTR_ENV_READ);

  if (lsstrcmp(name, lsstr_cstr("add")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.add"), 2, lsbuiltin_add, NULL,
                                    LSBATTR_PURE);
  if (lsstrcmp(name, lsstr_cstr("seq")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.seq"), 2, lsbuiltin_seq, NULL,
                                    LSBATTR_EFFECT);
  if (lsstrcmp(name, lsstr_cstr("print")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.print"), 1, lsbuiltin_print, NULL,
                                    LSBATTR_EFFECT);
  if (lsstrcmp(name, lsstr_cstr("println")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.println"), 1, pl_println, NULL,
                                    LSBATTR_EFFECT);
  if (lsstrcmp(name, lsstr_cstr("to_str")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.to_str"), 1, lsbuiltin_to_string, NULL,
                                    LSBATTR_PURE);
  if (lsstrcmp(name, lsstr_cstr("strcat")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.strcat"), 2, pl_strcat, NULL, LSBATTR_PURE);
  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.exit"), 1, pl_exit, NULL, LSBATTR_EFFECT);
  /* def removed */
  /* nsdef removed */
  if (lsstrcmp(name, lsstr_cstr("require")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.require"), 1, pl_require, tenv,
                                    LSBATTR_EFFECT | LSBATTR_ENV_READ);
  if (lsstrcmp(name, lsstr_cstr("requireOpt")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.requireOpt"), 1, pl_require_opt, tenv,
                                    LSBATTR_EFFECT | LSBATTR_ENV_READ);
  if (lsstrcmp(name, lsstr_cstr("include")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.include"), 1, pl_include, tenv,
                                    LSBATTR_ENV_READ);
  /* pluginHello removed */
  if (lsstrcmp(name, lsstr_cstr("importOpt")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.importOpt"), 1, pl_importOpt, tenv,
                                    LSBATTR_ENV_WRITE | LSBATTR_EFFECT);
  /* nsSelf removed */
  if (lsstrcmp(name, lsstr_cstr("withImport")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.withImport"), 2, pl_withImport, tenv,
                                    LSBATTR_ENV_WRITE | LSBATTR_EFFECT);
  if (lsstrcmp(name, lsstr_cstr("chain")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.chain"), 2, pl_chain, NULL);
  if (lsstrcmp(name, lsstr_cstr("bind")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.bind"), 2, pl_bind, NULL);
  if (lsstrcmp(name, lsstr_cstr("return")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.return"), 1, pl_return, NULL);
  if (lsstrcmp(name, lsstr_cstr("trace")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.trace"), 2, pl_trace, NULL, LSBATTR_EFFECT);
  if (lsstrcmp(name, lsstr_cstr("force")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.force"), 1, pl_force, NULL, LSBATTR_PURE);
  if (lsstrcmp(name, lsstr_cstr("traceForce")) == 0)
    return lsthunk_new_builtin_attr(lsstr_cstr("prelude.traceForce"), 2, pl_trace_force, NULL,
                                    LSBATTR_EFFECT);
  /* raise removed: use ^(expr) dedicated AST */
  if (lsstrcmp(name, lsstr_cstr("lt")) == 0) {
    lsthunk_t* carg     = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* cargv[1] = { carg };
    lsthunk_t* core_ns  = lsbuiltin_prelude_builtin(1, cargv, tenv);
    if (!core_ns)
      return ls_make_err("prelude.lt: no core builtins");
    lsthunk_t* key      = lsthunk_new_symbol(lsstr_cstr(".lt"));
    lsthunk_t* argv1[1] = { key };
    return lsthunk_eval(core_ns, 1, argv1);
  }
  if (lsstrcmp(name, lsstr_cstr("eq")) == 0) {
    lsthunk_t* carg     = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* cargv[1] = { carg };
    lsthunk_t* core_ns  = lsbuiltin_prelude_builtin(1, cargv, tenv);
    if (!core_ns)
      return ls_make_err("prelude.eq: no core builtins");
    lsthunk_t* key      = lsthunk_new_symbol(lsstr_cstr(".eq"));
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
  /* pluginHello removed */
  lsprintf(stderr, 0, "E: prelude: unknown symbol: ");
  lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
  lsprintf(stderr, 0, "\n");
  return ls_make_err("prelude: unknown symbol");
}

// Deprecated: user-facing prelude registration is disabled. Keep symbol for legacy loaders,
// but do not register names when called accidentally.
int ls_prelude_register(lstenv_t* tenv) {
  if (!tenv)
    return -1;
  (void)tenv;
  return 0; // no-op
}

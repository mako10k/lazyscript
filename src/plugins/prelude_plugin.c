#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "expr/ealge.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/effects.h"
#include "builtins/ns.h"
#include "builtins/builtin_loader.h"
#include "builtins/builtin_loader.h"
#include <stdlib.h>
#include <string.h>
#include <gc.h>
#include <assert.h>
#include "runtime/error.h"
#include "builtins/ns.h"

// forward decls used by pl_internal_dispatch
static lsthunk_t* pl_require(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_include(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_import(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_withImport(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* pl_def(lssize_t argc, lsthunk_t* const* args, void* data);

static lsthunk_t* pl_internal_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data; (void)argc;
  lsthunk_t* keyv = lsthunk_eval0(args[0]); if (keyv == NULL) return NULL;
  if (lsthunk_get_type(keyv) != LSTTYPE_SYMBOL) return ls_make_err("internal: expected symbol");
  const lsstr_t* s = lsthunk_get_symbol(keyv);
  if (lsstrcmp(s, lsstr_cstr(".require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, pl_require, tenv);
  if (lsstrcmp(s, lsstr_cstr(".include")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.include"), 1, pl_include, tenv);
  if (lsstrcmp(s, lsstr_cstr(".import")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.import"), 1, pl_import, tenv);
  if (lsstrcmp(s, lsstr_cstr(".withImport")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.withImport"), 2, pl_withImport, tenv);
  if (lsstrcmp(s, lsstr_cstr(".def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, pl_def, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsnew")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsnew"), 1, lsbuiltin_nsnew, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsdef")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdef"), 3, lsbuiltin_nsdef, tenv);
  if (lsstrcmp(s, lsstr_cstr(".nsnew0")) == 0) return lsbuiltin_nsnew0(0, NULL, tenv);
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

static lsthunk_t* ls_make_unit(void) {
  const lsealge_t* eunit = lsealge_new(lsstr_cstr("()"), 0, NULL);
  return lsthunk_new_ealge(eunit, NULL);
}

static lsthunk_t* pl_getter0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; return (lsthunk_t*)data;
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

static lsthunk_t* pl_println(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: println: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* thunk_str = lsthunk_eval0(args[0]);
  if (thunk_str == NULL)
    return NULL;
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR) {
    // Fallback: use to_str if available in env would be nicer; here we print thunk directly
    lsthunk_dprint(stdout, LSPREC_LOWEST, 0, thunk_str);
  } else {
    const lsstr_t* str = lsthunk_get_str(thunk_str);
    lsstr_print_bare(stdout, LSPREC_LOWEST, 0, str);
  }
  lsprintf(stdout, 0, "\n");
  return ls_make_unit();
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
    lsprintf(stderr, 0, "E: def: no env\n");
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

// Forward to host implementations for module loading
extern lsthunk_t* lsbuiltin_prelude_require(lssize_t, lsthunk_t* const*, void*);
extern lsthunk_t* lsbuiltin_prelude_include(lssize_t, lsthunk_t* const*, void*);

static lsthunk_t* pl_require(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  return lsbuiltin_prelude_require(argc, args, tenv);
}

static lsthunk_t* pl_include(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  return lsbuiltin_prelude_include(argc, args, tenv);
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
static lsthunk_t* pl_eq(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  // Evaluate both arguments (pure)
  lsthunk_t* a = lsthunk_eval0(args[0]); if (a == NULL) return NULL;
  lsthunk_t* b = lsthunk_eval0(args[1]); if (b == NULL) return NULL;
  // int equality
  if (lsthunk_get_type(a) == LSTTYPE_INT && lsthunk_get_type(b) == LSTTYPE_INT) {
    int eq = lsint_eq(lsthunk_get_int(a), lsthunk_get_int(b));
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  // symbol equality (explicit symbol kind)
  if (lsthunk_get_type(a) == LSTTYPE_SYMBOL && lsthunk_get_type(b) == LSTTYPE_SYMBOL) {
    const lsstr_t* sa = lsthunk_get_symbol(a);
    const lsstr_t* sb = lsthunk_get_symbol(b);
    int eq = (lsstrcmp(sa, sb) == 0);
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  // algebraic 0-arity constructors (e.g., .a style)
  if (lsthunk_get_type(a) == LSTTYPE_ALGE && lsthunk_get_argc(a) == 0 &&
      lsthunk_get_type(b) == LSTTYPE_ALGE && lsthunk_get_argc(b) == 0) {
    const lsstr_t* ca = lsthunk_get_constr(a);
    const lsstr_t* cb = lsthunk_get_constr(b);
    int eq = (lsstrcmp(ca, cb) == 0);
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("false"), 0, NULL), NULL);
}

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

static lsthunk_t* pl_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  lstenv_t* tenv = (lstenv_t*)data;
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_t* key = lsthunk_eval0(args[0]);
  if (key == NULL)
    return NULL;
  if (lsthunk_get_type(key) != LSTTYPE_ALGE || lsthunk_get_argc(key) != 0) {
    lsprintf(stderr, 0, "E: prelude: expected a bare symbol (exit/println/chain/bind/return)\n");
    return NULL;
  }
  // Bind ~builtins once for this prelude environment
  static int prelude_builtins_bound = 0;
  if (!prelude_builtins_bound && tenv) {
    lsthunk_t* arg = lsthunk_new_str(lsstr_cstr("core"));
    lsthunk_t* argv1[1] = { arg };
    lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, argv1, tenv);
    if (core_ns) {
  lstenv_put_builtin(tenv, lsstr_cstr("builtins"), 0, pl_getter0, core_ns);
  lsthunk_t* internal_ns = lsthunk_new_builtin(lsstr_cstr("prelude.internal"), 1, pl_internal_dispatch, tenv);
      lstenv_put_builtin(tenv, lsstr_cstr("internal"), 0, pl_getter0, internal_ns);
      prelude_builtins_bound = 1;
    }
  }
  const lsstr_t* name = lsthunk_get_constr(key);
  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.exit"), 1, pl_exit, NULL);
  if (lsstrcmp(name, lsstr_cstr("println")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.println"), 1, pl_println, NULL);
  if (lsstrcmp(name, lsstr_cstr("def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, pl_def, tenv);
  if (lsstrcmp(name, lsstr_cstr("require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, pl_require, tenv);
  if (lsstrcmp(name, lsstr_cstr("include")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.include"), 1, pl_include, tenv);
  if (lsstrcmp(name, lsstr_cstr("import")) == 0 || lsstrcmp(name, lsstr_cstr(".import")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.import"), 1, pl_import, tenv);
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
  if (lsstrcmp(name, lsstr_cstr("nsnew")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsnew"), 1, lsbuiltin_nsnew, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsdef")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdef"), 3, lsbuiltin_nsdef, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsnew0")) == 0) {
    if (!ls_effects_allowed()) {
      lsprintf(stderr, 0, "E: nsnew0: effect used in pure context (enable seq/chain)\n");
      return NULL;
    }
    return lsbuiltin_nsnew0(0, NULL, tenv);
  }
  if (lsstrcmp(name, lsstr_cstr("nsdefv")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdefv"), 3, lsbuiltin_nsdefv, tenv);
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
  // Return an error thunk instead of NULL to avoid downstream segfaults
  return ls_make_err("prelude: unknown symbol");
}

int ls_prelude_register(lstenv_t* tenv) {
  if (!tenv)
    return -1;
  lstenv_put_builtin(tenv, lsstr_cstr("prelude"), 1, pl_dispatch, tenv);
  return 0;
}

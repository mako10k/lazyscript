#include "builtins/ns.h"
#include "common/hash.h"
#include "common/malloc.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/unit.h"
#include "thunk/tenv.h"
#include "runtime/error.h"

// simple namespace object
typedef struct lsns { lshash_t* map; } lsns_t;

// Debug logging: define LS_NS_DEBUG=1 at compile time to enable
#ifndef LS_NS_DEBUG
#define LS_NS_DEBUG 0
#endif
#if LS_NS_DEBUG
#define NS_DLOG_ENABLED 1
#else
#define NS_DLOG_ENABLED 0
#endif

// Forward decls
static lsthunk_t* lsbuiltin_ns_dispatch(lssize_t argc, lsthunk_t* const* args, void* data);
static lsthunk_t* lsbuiltin_ns_set(lssize_t argc, lsthunk_t* const* args, void* data);
// Represent an ns value as a 1-ary builtin that dispatches on symbol: (~NS sym)
static lshash_t* g_namespaces = NULL; // name -> ns* (named namespaces only)
lsthunk_t* lsbuiltin_ns_value(lssize_t argc, lsthunk_t* const* args, void* data) {
  // data directly holds the namespace instance
  lsns_t* ns = (lsns_t*)data;
  if (!ns) return NULL;
  return lsbuiltin_ns_dispatch(argc, args, ns);
}

static lsthunk_t* lsbuiltin_ns_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lsns_t* ns = (lsns_t*)data;
  if (!ns || !ns->map) return ls_make_err("namespace: invalid");
  lsthunk_t* keyv = ls_eval_arg(args[0], "namespace: key");
  if (lsthunk_is_err(keyv)) return keyv;
  if (lsthunk_get_type(keyv) != LSTTYPE_ALGE || lsthunk_get_argc(keyv) != 0) {
    return ls_make_err("namespace: expected bare symbol");
  }
  const lsstr_t* key = lsthunk_get_constr(keyv);
  // optional debug
  if (NS_DLOG_ENABLED) {
    lsprintf(stderr, 0, "DBG ns_lookup ns=%p ", (void*)ns);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, key);
    lsprintf(stderr, 0, "\n");
  }
  // Special setter: (~NS __set) -> builtin (sym val) that defines into NS and returns ()
  if (lsstrcmp(key, lsstr_cstr("__set")) == 0) {
    return lsthunk_new_builtin(lsstr_cstr("namespace.__set"), 2, lsbuiltin_ns_set, ns);
  }
  lshash_data_t  valp;
  if (!lshash_get(ns->map, key, &valp)) {
    return ls_make_err("namespace: undefined");
  }
  if (NS_DLOG_ENABLED) {
    lsthunk_t* tv = (lsthunk_t*)valp;
    const char* tt = "?";
    if (tv) {
      switch (lsthunk_get_type(tv)) {
      case LSTTYPE_LAMBDA: tt = "lambda"; break;
      case LSTTYPE_INT: tt = "int"; break;
      case LSTTYPE_STR: tt = "str"; break;
      case LSTTYPE_ALGE: tt = "alge"; break;
      case LSTTYPE_APPL: tt = "appl"; break;
      case LSTTYPE_REF: tt = "ref"; break;
      case LSTTYPE_BUILTIN: tt = "builtin"; break;
      case LSTTYPE_CHOICE: tt = "choice"; break;
      }
    }
    lsprintf(stderr, 0, "DBG ns_lookup ret type=%s\n", tt);
  }
  return (lsthunk_t*)valp;
}

// __set helper implementation
static lsthunk_t* lsbuiltin_ns_set(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lsns_t* ns = (lsns_t*)data; if (!ns || !ns->map) return ls_make_err("namespace: invalid");
  lsthunk_t* symv = ls_eval_arg(args[0], "namespace: __set key");
  if (lsthunk_is_err(symv)) return symv;
  if (lsthunk_get_type(symv) != LSTTYPE_ALGE || lsthunk_get_argc(symv) != 0) {
    return ls_make_err("namespace: __set expects bare symbol");
  }
  const lsstr_t* s = lsthunk_get_constr(symv);
  lsthunk_t* val = ls_eval_arg(args[1], "namespace: __set value");
  if (lsthunk_is_err(val)) return val;
  lshash_data_t oldv; (void)lshash_put(ns->map, s, (const void*)val, &oldv);
  return ls_make_unit();
}

// global registry name -> ns*
static unsigned long g_ns_counter = 0;
static const lsstr_t* ns_make_key(const lsstr_t* base) {
  if (base) return base;
  char buf[32];
  snprintf(buf, sizeof(buf), "__ns$%lu", ++g_ns_counter);
  return lsstr_cstr(buf);
}

lsthunk_t* lsbuiltin_nsnew(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return NULL;
  lsthunk_t* namev = ls_eval_arg(args[0], "nsnew: name");
  if (lsthunk_is_err(namev)) return namev;
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    return ls_make_err("nsnew: expected bare symbol");
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  lsns_t* ns = lsmalloc(sizeof(lsns_t)); ns->map = lshash_new(16);
  if (!g_namespaces) g_namespaces = lshash_new(16);
  lshash_data_t oldv; (void)lshash_put(g_namespaces, name, (const void*)ns, &oldv);
  // Bind named namespace as (~Name sym)
  lstenv_put_builtin(tenv, name, 1, lsbuiltin_ns_dispatch, ns);
  // Also construct a value for possible retrieval (not returned here)
  lsthunk_t* nsval = lsthunk_new_builtin(lsstr_cstr("namespace"), 1, lsbuiltin_ns_value, (void*)ns);
  // Return unit for nsnew (named variant) for backward compatibility
  (void)nsval; // not used by caller
  return ls_make_unit();
}

lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lstenv_t* tenv = (lstenv_t*)data; (void)tenv; // unused
  lsthunk_t* nsv = ls_eval_arg(args[0], "nsdef: ns");
  if (lsthunk_is_err(nsv)) return nsv;
  lsthunk_t* symv = ls_eval_arg(args[1], "nsdef: key");
  if (lsthunk_is_err(symv)) return symv;
  if (lsthunk_get_type(nsv) != LSTTYPE_ALGE || lsthunk_get_argc(nsv) != 0) {
    return ls_make_err("nsdef: expected ns symbol");
  }
  if (lsthunk_get_type(symv) != LSTTYPE_ALGE || lsthunk_get_argc(symv) != 0) {
    return ls_make_err("nsdef: expected name symbol");
  }
  const lsstr_t* nsname  = lsthunk_get_constr(nsv);
  const lsstr_t* symname = lsthunk_get_constr(symv);
  if (!g_namespaces) {
    lsprintf(stderr, 0, "E: nsdef: namespace not found: "); lsstr_print_bare(stderr, LSPREC_LOWEST, 0, nsname); lsprintf(stderr, 0, "\n");
    return NULL;
  }
  lshash_data_t nsp;
  if (!lshash_get(g_namespaces, nsname, &nsp)) {
    lsprintf(stderr, 0, "E: nsdef: namespace not found: "); lsstr_print_bare(stderr, LSPREC_LOWEST, 0, nsname); lsprintf(stderr, 0, "\n");
    return NULL;
  }
  lsns_t* ns = (lsns_t*)nsp; if (!ns || !ns->map) return ls_make_err("nsdef: invalid namespace");
  lsthunk_t* val = ls_eval_arg(args[2], "nsdef: value");
  if (lsthunk_is_err(val)) return val;
  lshash_data_t oldv; (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  return ls_make_unit();
}

// Create an anonymous namespace value: returns NS value usable as (~NS sym)
lsthunk_t* lsbuiltin_nsnew0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; (void)data;
  lsns_t* ns = lsmalloc(sizeof(lsns_t)); ns->map = lshash_new(16);
  if (NS_DLOG_ENABLED) {
    lsprintf(stderr, 0, "DBG nsnew0 ns=%p map=%p\n", (void*)ns, (void*)ns->map);
  }
  // For anonymous namespaces, we return the value directly; no need to register
  return lsthunk_new_builtin(lsstr_cstr("namespace"), 1, lsbuiltin_ns_value, (void*)ns);
}

// Define into a namespace value directly: (nsdefv NS sym value)
lsthunk_t* lsbuiltin_nsdefv(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  lsthunk_t* nsv = ls_eval_arg(args[0], "nsdefv: ns");
  if (lsthunk_is_err(nsv)) return nsv;
  // Accept either a namespace value (builtin carrying ns pointer) or a bare symbol
  lsns_t* ns = NULL;
  if (lsthunk_is_builtin(nsv)) {
    // Verify it's our namespace-value builtin
    lstbuiltin_func_t fn = lsthunk_get_builtin_func(nsv);
    if (fn == lsbuiltin_ns_value) {
      ns = (lsns_t*)lsthunk_get_builtin_data(nsv);
    }
  } else if (lsthunk_get_type(nsv) == LSTTYPE_ALGE && lsthunk_get_argc(nsv) == 0) {
    const lsstr_t* nsname = lsthunk_get_constr(nsv);
    if (g_namespaces) { lshash_data_t nsp; if (lshash_get(g_namespaces, nsname, &nsp)) ns = (lsns_t*)nsp; }
  }
  // silently accept resolved namespace
  if (!ns || !ns->map) {
    return ls_make_err("nsdefv: invalid namespace");
  }
  lsthunk_t* symv = ls_eval_arg(args[1], "nsdefv: key");
  if (lsthunk_is_err(symv)) return symv;
  if (lsthunk_get_type(symv) != LSTTYPE_ALGE || lsthunk_get_argc(symv) != 0) {
    return ls_make_err("nsdefv: expected bare symbol");
  }
  const lsstr_t* symname = lsthunk_get_constr(symv);
  lsthunk_t* val = ls_eval_arg(args[2], "nsdefv: value");
  if (lsthunk_is_err(val)) return val;
  if (NS_DLOG_ENABLED) {
    lsprintf(stderr, 0, "DBG nsdefv put ns=%p ", (void*)ns);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, symname);
    lsprintf(stderr, 0, "\n");
  }
  // insert into namespace map
  lshash_data_t oldv; (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  return ls_make_unit();
}

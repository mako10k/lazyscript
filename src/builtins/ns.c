#include "builtins/ns.h"
#include "common/hash.h"
#include "common/malloc.h"
#include "common/str.h"
#include "common/io.h"
#include "common/int.h"
#include "runtime/unit.h"
#include "runtime/effects.h"
#include "thunk/tenv.h"
#include "runtime/error.h"
#include "expr/expr.h"
#include <stdlib.h>
#include <string.h>

// simple namespace object
typedef struct lsns {
  lshash_t* map;
  int       is_mutable; // 1: mutable (nsnew/nsnew0), 0: immutable (nslit)
} lsns_t;

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

// ----------------------------------------
// Current namespace during nslit evaluation (stacked to allow nesting)
static lsns_t* g_nslit_self = NULL;

// (~prelude nsSelf) => returns the namespace value currently being built by nslit
lsthunk_t* lsbuiltin_prelude_ns_self(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; (void)data;
  if (!g_nslit_self || !g_nslit_self->map) {
    return ls_make_err("nsSelf: not in namespace literal");
  }
  return lsthunk_new_builtin(lsstr_cstr("namespace"), 1, lsbuiltin_ns_value, g_nslit_self);
}
// Key canonicalization
// Supports keys of type: Symbol only. Encoded as a string with leading 'S'
// to avoid collisions. Returns NULL on invalid type and reports a descriptive
// error via stderr.
static const lsstr_t* ns_encode_key_from_thunk(lsthunk_t* keyv) {
  if (!keyv) return NULL;
  if (lsthunk_get_type(keyv) == LSTTYPE_SYMBOL) {
    const lsstr_t* s = lsthunk_get_symbol(keyv);
    lssize_t n = lsstr_get_len(s);
    const char* p = lsstr_get_buf(s);
    char* buf = lsmalloc((size_t)n + 1);
    buf[0] = 'S';
    for (lssize_t i = 0; i < n; ++i) buf[1 + i] = p[i];
    const lsstr_t* k = lsstr_new(buf, n + 1);
    lsfree(buf);
    return k;
  }
  lsprintf(stderr, 0, "E: namespace: key must be symbol\n");
  return NULL;
}

// ----------------------------------------
// nsMembers implementation helpers
typedef struct ns_key_item {
  const char*  bytes; // pointer to key bytes after tag
  lssize_t     len;   // length after tag
} ns_key_item_t;

typedef struct ns_collect_ctx {
  ns_key_item_t* items;
  size_t         count;
  size_t         cap;
} ns_collect_ctx_t;

static void ns_collect_cb(const lsstr_t* key, lshash_data_t value, void* data) {
  (void)value;
  ns_collect_ctx_t* ctx = (ns_collect_ctx_t*)data;
  if (!key) return;
  const char* bytes = lsstr_get_buf(key);
  lssize_t    len   = lsstr_get_len(key);
  if (!bytes || len <= 0) return;
  if (ctx->count == ctx->cap) {
    size_t ncap = ctx->cap ? ctx->cap * 2 : 16;
    ctx->items  = (ns_key_item_t*)lsrealloc(ctx->items, ncap * sizeof(ns_key_item_t));
    ctx->cap    = ncap;
  }
  if (bytes[0] != 'S') return; // only symbol keys are expected
  ns_key_item_t it;
  it.bytes = bytes + 1;
  it.len   = len - 1;
  ctx->items[ctx->count++] = it;
}

static int ns_item_cmp(const void* a, const void* b) {
  const ns_key_item_t* x = (const ns_key_item_t*)a;
  const ns_key_item_t* y = (const ns_key_item_t*)b;
  int min = (int)(x->len < y->len ? x->len : y->len);
  int c   = memcmp(x->bytes, y->bytes, (size_t)min);
  if (c != 0) return c;
  // shorter first
  if (x->len != y->len) return (int)(x->len - y->len);
  return 0;
}

// build cons list from an array of thunks: elems[0] : (elems[1] : ... [] )
// Convert a collected key item back into an expression representing its const value
static const lsexpr_t* ns_item_to_expr(const ns_key_item_t* it) {
  // Symbol literal as a zero-arity constructor (e.g., .name)
  const lsealge_t* ea = lsealge_new(lsstr_new(it->bytes, it->len), 0, NULL);
  return lsexpr_new_alge(ea);
}

// Build cons-list expression from collected items then thunk it once at the end
static lsthunk_t* ns_build_list(size_t n, const ns_key_item_t* items) {
  const lsexpr_t* tail = lsexpr_new_alge(lsealge_new(lsstr_cstr("[]"), 0, NULL));
  for (long i = (long)n - 1; i >= 0; --i) {
    const lsexpr_t* head = ns_item_to_expr(&items[i]);
    const lsexpr_t* args2[2] = { head, tail };
    tail = lsexpr_new_alge(lsealge_new(lsstr_cstr(":"), 2, args2));
  }
  return lsthunk_new_expr(tail, NULL);
}


// (~nsMembers ns) => list<symbol>
lsthunk_t* lsbuiltin_ns_members(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  // Accept namespace value or named symbol
  lsthunk_t* nsv = ls_eval_arg(args[0], "nsMembers: ns");
  if (lsthunk_is_err(nsv)) return nsv;
  if (!nsv) return ls_make_err("nsMembers: ns eval");
  lsns_t* ns = NULL;
  if (lsthunk_is_builtin(nsv)) {
    lstbuiltin_func_t fn = lsthunk_get_builtin_func(nsv);
    if (fn == lsbuiltin_ns_value) ns = (lsns_t*)lsthunk_get_builtin_data(nsv);
  } else if (lsthunk_get_type(nsv) == LSTTYPE_ALGE && lsthunk_get_argc(nsv) == 0) {
    const lsstr_t* nsname = lsthunk_get_constr(nsv);
    if (g_namespaces) { lshash_data_t nsp; if (lshash_get(g_namespaces, nsname, &nsp)) ns = (lsns_t*)nsp; }
  }
  if (!ns || !ns->map) return ls_make_err("nsMembers: not a namespace");

  ns_collect_ctx_t ctx = {0};
  lshash_foreach(ns->map, ns_collect_cb, &ctx);
  if (ctx.count == 0) {
    const lsealge_t* nil = lsealge_new(lsstr_cstr("[]"), 0, NULL);
    return lsthunk_new_ealge(nil, NULL);
  }
  qsort(ctx.items, ctx.count, sizeof(ns_key_item_t), ns_item_cmp);
  lsthunk_t* list = ns_build_list(ctx.count, ctx.items);
  lsfree(ctx.items);
  return list;
}

// Public iteration helper used by prelude.import and others
typedef struct {
  lsns_iter_cb cb;
  void*        data;
} ns_iter_user_cb_t;

static void ns_iter_cb_adapter(const lsstr_t* key, lshash_data_t value, void* data) {
  ns_iter_user_cb_t* u = (ns_iter_user_cb_t*)data;
  if (!u || !u->cb || !key) return;
  const char* bytes = lsstr_get_buf(key);
  lssize_t    len   = lsstr_get_len(key);
  if (!bytes || len <= 0) return;
  if (bytes[0] != 'S') return; // only symbol keys
  const lsstr_t* sym = lsstr_new(bytes + 1, len - 1);
  u->cb(sym, (lsthunk_t*)value, u->data);
}

int lsns_foreach_member(lsthunk_t* ns_thunk, lsns_iter_cb cb, void* data) {
  if (!ns_thunk || !cb) return 0;
  // Accept either namespace value or named symbol (0-arity constructor)
  lsns_t* ns = NULL;
  if (lsthunk_is_builtin(ns_thunk)) {
    lstbuiltin_func_t fn = lsthunk_get_builtin_func(ns_thunk);
    if (fn == lsbuiltin_ns_value) ns = (lsns_t*)lsthunk_get_builtin_data(ns_thunk);
  } else if (lsthunk_get_type(ns_thunk) == LSTTYPE_ALGE && lsthunk_get_argc(ns_thunk) == 0) {
    const lsstr_t* nsname = lsthunk_get_constr(ns_thunk);
    if (g_namespaces) { lshash_data_t nsp; if (lshash_get(g_namespaces, nsname, &nsp)) ns = (lsns_t*)nsp; }
  }
  if (!ns || !ns->map) return 0;
  ns_iter_user_cb_t u = { cb, data };
  lshash_foreach(ns->map, ns_iter_cb_adapter, &u);
  return 1;
}

static lsthunk_t* lsbuiltin_ns_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lsns_t* ns = (lsns_t*)data;
  if (!ns || !ns->map) return ls_make_err("namespace: invalid");
  lsthunk_t* keyv = ls_eval_arg(args[0], "namespace: key");
  if (lsthunk_is_err(keyv)) return keyv;
  if (!keyv) return ls_make_err("namespace: key eval");
  // Suggestion for common mistake: passing string "__set"/".__set"
  if (lsthunk_get_type(keyv) == LSTTYPE_STR) {
    const lsstr_t* s = lsthunk_get_str(keyv);
    if (lsstrcmp(s, lsstr_cstr("__set")) == 0 || lsstrcmp(s, lsstr_cstr(".__set")) == 0) {
      return ls_make_err("namespace: use (~ns __set) or (~ns .__set) for setter");
    }
  }
  // Setter: allow only symbol .__set
  if (lsthunk_get_type(keyv) == LSTTYPE_SYMBOL) {
    const lsstr_t* s = lsthunk_get_symbol(keyv);
    if (lsstrcmp(s, lsstr_cstr(".__set")) == 0) {
      return lsthunk_new_builtin(lsstr_cstr("namespace.__set"), 2, lsbuiltin_ns_set, ns);
    }
  }
  const lsstr_t* key = ns_encode_key_from_thunk(keyv);
  if (!key) return ls_make_err("namespace: symbol expected");
  // optional debug
  if (NS_DLOG_ENABLED) {
    lsprintf(stderr, 0, "DBG ns_lookup ns=%p ", (void*)ns);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, key);
    lsprintf(stderr, 0, "\n");
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
  case LSTTYPE_SYMBOL: tt = "symbol"; break;
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
  if (!ns->is_mutable) {
    return ls_make_err("namespace: immutable");
  }
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: namespace.__set: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* keyv = ls_eval_arg(args[0], "namespace: __set key");
  if (lsthunk_is_err(keyv)) return keyv;
  if (!keyv) return ls_make_err("namespace: __set key eval");
  const lsstr_t* s = ns_encode_key_from_thunk(keyv);
  if (!s) return ls_make_err("namespace: symbol expected");
  lsthunk_t* val = ls_eval_arg(args[1], "namespace: __set value");
  if (lsthunk_is_err(val)) return val;
  if (!val) return ls_make_err("namespace: __set value eval");
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
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: nsnew: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* namev = ls_eval_arg(args[0], "nsnew: name");
  if (lsthunk_is_err(namev)) return namev;
  if (!namev) return ls_make_err("nsnew: name eval");
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    return ls_make_err("nsnew: expected bare symbol");
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  if (!g_namespaces) g_namespaces = lshash_new(16);
  lsns_t* ns = lsmalloc(sizeof(lsns_t)); ns->map = lshash_new(16); ns->is_mutable = 1;
  lshash_data_t oldv; (void)lshash_put(g_namespaces, name, (const void*)ns, &oldv);
  // Bind named namespace as (~Name sym)
  lstenv_put_builtin(tenv, name, 1, lsbuiltin_ns_dispatch, ns);
  return ls_make_unit();
}

lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lstenv_t* tenv = (lstenv_t*)data; (void)tenv; // unused
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: nsdef: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* nsv = ls_eval_arg(args[0], "nsdef: ns");
  if (lsthunk_is_err(nsv)) return nsv;
  lsthunk_t* symv = ls_eval_arg(args[1], "nsdef: key");
  if (lsthunk_is_err(symv)) return symv;
  if (!nsv || !symv) return ls_make_err("nsdef: arg eval");
  if (lsthunk_get_type(nsv) != LSTTYPE_ALGE || lsthunk_get_argc(nsv) != 0) {
    return ls_make_err("nsdef: expected ns symbol");
  }
  const lsstr_t* nsname  = lsthunk_get_constr(nsv);
  const lsstr_t* symname = ns_encode_key_from_thunk(symv);
  if (!symname) return ls_make_err("nsdef: symbol expected");
  if (!g_namespaces) {
    return ls_make_err("nsdef: unknown namespace");
  }
  lshash_data_t nsp;
  if (!lshash_get(g_namespaces, nsname, &nsp)) {
    return ls_make_err("nsdef: unknown namespace");
  }
  lsns_t* ns = (lsns_t*)nsp; if (!ns || !ns->map) return ls_make_err("nsdef: invalid namespace");
  if (!ns->is_mutable) return ls_make_err("nsdef: immutable namespace");
  lsthunk_t* val = ls_eval_arg(args[2], "nsdef: value");
  if (lsthunk_is_err(val)) return val;
  if (!val) return ls_make_err("nsdef: value eval");
  lshash_data_t oldv; (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  return ls_make_unit();
}

// Create an anonymous namespace value: returns NS value usable as (~NS sym)
lsthunk_t* lsbuiltin_nsnew0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; (void)data;
  lsns_t* ns = lsmalloc(sizeof(lsns_t)); ns->map = lshash_new(16); ns->is_mutable = 1;
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
  if (!nsv) return ls_make_err("nsdefv: ns eval");
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
  if (!ns->is_mutable) {
    return ls_make_err("nsdefv: immutable namespace");
  }
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: nsdefv: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* symv = ls_eval_arg(args[1], "nsdefv: key");
  if (lsthunk_is_err(symv)) return symv;
  if (!symv) return ls_make_err("nsdefv: key eval");
  const lsstr_t* symname = ns_encode_key_from_thunk(symv);
  if (!symname) return ls_make_err("nsdefv: symbol expected");
  lsthunk_t* val = ls_eval_arg(args[2], "nsdefv: value");
  if (lsthunk_is_err(val)) return val;
  if (!val) return ls_make_err("nsdefv: value eval");
  if (NS_DLOG_ENABLED) {
    lsprintf(stderr, 0, "DBG nsdefv put ns=%p ", (void*)ns);
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, symname);
    lsprintf(stderr, 0, "\n");
  }
  // insert into namespace map
  lshash_data_t oldv; (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  return ls_make_unit();
}

// Construct a namespace value from literal pairs: (nslit sym1 val1 sym2 val2 ...)
lsthunk_t* lsbuiltin_nslit(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  if (argc % 2 != 0) {
    return ls_make_err("nslit: expected even number of arguments");
  }
  lsns_t* ns = lsmalloc(sizeof(lsns_t));
  ns->map = lshash_new(16);
  ns->is_mutable = 0; // immutable literal namespace
  // Enable (~prelude nsSelf) inside values while building
  lsns_t* prev_self = g_nslit_self; g_nslit_self = ns;
  for (lssize_t i = 0; i < argc; i += 2) {
    lsthunk_t* symv = ls_eval_arg(args[i], "nslit: key");
    if (lsthunk_is_err(symv)) { g_nslit_self = prev_self; return symv; }
    const lsstr_t* symname = ns_encode_key_from_thunk(symv);
    if (!symname) { g_nslit_self = prev_self; return ls_make_err("nslit: symbol expected"); }
    lsthunk_t* val = ls_eval_arg(args[i + 1], "nslit: value");
    if (lsthunk_is_err(val)) { g_nslit_self = prev_self; return val; }
    lshash_data_t oldv; (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  }
  g_nslit_self = prev_self;
  return lsthunk_new_builtin(lsstr_cstr("namespace"), 1, lsbuiltin_ns_value, (void*)ns);
}

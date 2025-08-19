#include "builtins/ns.h"
#include "common/hash.h"
#include "common/malloc.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/unit.h"
#include "thunk/tenv.h"

// simple namespace object
typedef struct lsns { lshash_t* map; } lsns_t;

static lsthunk_t* lsbuiltin_ns_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lsns_t* ns = (lsns_t*)data;
  if (!ns || !ns->map) return NULL;
  lsthunk_t* keyv = lsthunk_eval0(args[0]); if (!keyv) return NULL;
  if (lsthunk_get_type(keyv) != LSTTYPE_ALGE || lsthunk_get_argc(keyv) != 0) {
    lsprintf(stderr, 0, "E: namespace: expected bare symbol\n");
    return NULL;
  }
  const lsstr_t* key = lsthunk_get_constr(keyv);
  lshash_data_t  valp;
  if (!lshash_get(ns->map, key, &valp)) {
    lsprintf(stderr, 0, "E: namespace: undefined: ");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, key);
    lsprintf(stderr, 0, "\n");
    return NULL;
  }
  return (lsthunk_t*)valp;
}

// global registry name -> ns*
static lshash_t* g_namespaces = NULL;

lsthunk_t* lsbuiltin_nsnew(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return NULL;
  lsthunk_t* namev = lsthunk_eval0(args[0]); if (!namev) return NULL;
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    lsprintf(stderr, 0, "E: nsnew: expected bare symbol\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  lsns_t* ns = lsmalloc(sizeof(lsns_t)); ns->map = lshash_new(16);
  lstenv_put_builtin(tenv, name, 1, lsbuiltin_ns_dispatch, ns);
  if (!g_namespaces) g_namespaces = lshash_new(16);
  lshash_data_t oldv; (void)lshash_put(g_namespaces, name, (const void*)ns, &oldv);
  return ls_make_unit();
}

lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; lstenv_t* tenv = (lstenv_t*)data; (void)tenv; // unused
  lsthunk_t* nsv = lsthunk_eval0(args[0]);
  lsthunk_t* symv = lsthunk_eval0(args[1]);
  if (!nsv || !symv) return NULL;
  if (lsthunk_get_type(nsv) != LSTTYPE_ALGE || lsthunk_get_argc(nsv) != 0) {
    lsprintf(stderr, 0, "E: nsdef: expected bare symbol for namespace\n");
    return NULL;
  }
  if (lsthunk_get_type(symv) != LSTTYPE_ALGE || lsthunk_get_argc(symv) != 0) {
    lsprintf(stderr, 0, "E: nsdef: expected bare symbol name\n");
    return NULL;
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
  lsns_t* ns = (lsns_t*)nsp; if (!ns || !ns->map) return NULL;
  lsthunk_t* val = lsthunk_eval0(args[2]); if (!val) return NULL;
  lshash_data_t oldv; (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  return ls_make_unit();
}

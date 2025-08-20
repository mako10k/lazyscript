#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "expr/ealge.h"
#include "common/str.h"
#include "common/io.h"
#include <stdlib.h>
#include <gc.h>
#include <assert.h>

static lsthunk_t* ls_make_unit(void) {
  const lsealge_t* eunit = lsealge_new(lsstr_cstr("()"), 0, NULL);
  return lsthunk_new_ealge(eunit, NULL);
}

static lsthunk_t* pl_exit(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
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

static lsthunk_t* pl_chain(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* action = lsthunk_eval0(args[0]);
  if (action == NULL)
    return NULL;
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

static lsthunk_t* pl_bind(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* val = lsthunk_eval0(args[0]);
  if (val == NULL)
    return NULL;
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &val);
}

static lsthunk_t* pl_return(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  return args[0];
}

static lsthunk_t* pl_plugin_hello(void) {
  return lsthunk_new_str(lsstr_cstr("plugin"));
}

static lsthunk_t* pl_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_t* key = lsthunk_eval0(args[0]);
  if (key == NULL)
    return NULL;
  if (lsthunk_get_type(key) != LSTTYPE_ALGE || lsthunk_get_argc(key) != 0) {
    lsprintf(stderr, 0, "E: prelude: expected a bare symbol (exit/println/chain/bind/return)\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(key);
  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.exit"), 1, pl_exit, NULL);
  if (lsstrcmp(name, lsstr_cstr("println")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.println"), 1, pl_println, NULL);
  if (lsstrcmp(name, lsstr_cstr("chain")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.chain"), 2, pl_chain, NULL);
  if (lsstrcmp(name, lsstr_cstr("bind")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.bind"), 2, pl_bind, NULL);
  if (lsstrcmp(name, lsstr_cstr("return")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.return"), 1, pl_return, NULL);
  if (lsstrcmp(name, lsstr_cstr("pluginHello")) == 0)
    return pl_plugin_hello();
  lsprintf(stderr, 0, "E: prelude: unknown symbol: ");
  lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
  lsprintf(stderr, 0, "\n");
  return NULL;
}

int ls_prelude_register(lstenv_t* tenv) {
  if (!tenv)
    return -1;
  lstenv_put_builtin(tenv, lsstr_cstr("prelude"), 1, pl_dispatch, NULL);
  return 0;
}

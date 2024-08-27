#include "tclosure.h"
#include "malloc.h"
#include "thunk.h"
#include <assert.h>

struct lstclosure {
  lstenv_t *ltc_env;
  const lseclosure_t *ltc_closure;
};

lstclosure_t *lstclosure_new(lstenv_t *tenv, const lseclosure_t *eclosure) {
  assert(tenv != NULL);
  assert(eclosure != NULL);
  lstclosure_t *tclosure = lsmalloc(sizeof(lstclosure_t));
  tclosure->ltc_env = lstenv(tenv);
  tclosure->ltc_closure = eclosure;
  return tclosure;
}

lsthunk_t *lstclosure_eval(lstclosure_t *tclosure) {
  assert(tclosure != NULL);
  return lsthunk_closure(tclosure);
}

lsthunk_t *lstclosure_apply(lstclosure_t *tclosure, const lstlist_t *args) {
  assert(tclosure != NULL);
  assert(args != NULL);
  return lseclosure_thunk(tclosure->ltc_env, tclosure->ltc_closure);
}
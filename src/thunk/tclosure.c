#include "thunk/tclosure.h"
#include "common/malloc.h"
#include "thunk/thunk.h"
#include <assert.h>

struct lstclosure {
  const lseclosure_t *ltc_closure;
};

lstclosure_t *lstclosure_new(const lseclosure_t *eclosure) {
  assert(eclosure != NULL);
  lstclosure_t *tclosure = lsmalloc(sizeof(lstclosure_t));
  tclosure->ltc_closure = eclosure;
  return tclosure;
}

lsthunk_t *lstclosure_eval(lstclosure_t *tclosure) {
  assert(tclosure != NULL);
  return lsthunk_new_closure(tclosure);
}

lsthunk_t *lstclosure_apply(lstclosure_t *tclosure, const lstlist_t *args) {
  assert(tclosure != NULL);
  assert(args != NULL);
  return NULL;
}
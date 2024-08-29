#include "thunk/tlambda.h"
#include "common/malloc.h"
#include <assert.h>

struct lstlambda {
  const lselambda_t *ltl_lambda;
};

lstlambda_t *lstlambda_new(const lselambda_t *elambda) {
  assert(elambda != NULL);
  lstlambda_t *tlambda = lsmalloc(sizeof(lstlambda_t));
  tlambda->ltl_lambda = elambda;
  return tlambda;
}

lsthunk_t *lstlambda_apply(lstlambda_t *tlambda, const lstlist_t *args) {
  return NULL; // TODO: implement
}
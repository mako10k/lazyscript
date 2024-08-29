#include "thunk/tlambda.h"
#include "common/malloc.h"
#include "expr/elambda.h"
#include "pat/pat.h"
#include "thunk/tref.h"
#include <assert.h>

struct lstlambda {
  const lspat_t *ltl_arg;
  const lsthunk_t *ltl_body;
};

lstlambda_t *lstlambda_new(const lselambda_t *elambda, lstenv_t *tenv) {
  assert(elambda != NULL);
  const lspat_t *arg = lselambda_get_arg(elambda);
  tenv = lstenv_new(tenv);
  const lstref_target_t *target = lstref_target_new_lambda(elambda, tenv);
  if (target == NULL)
    return NULL;
  lsthunk_t *body = lsthunk_new_expr(lselambda_get_body(elambda), tenv);
  if (body == NULL)
    return NULL;
  lstlambda_t *tlambda = lsmalloc(sizeof(lstlambda_t));
  tlambda->ltl_arg = arg;
  tlambda->ltl_body = body;
  return tlambda;
}

lsthunk_t *lstlambda_apply(lstlambda_t *tlambda, const lstlist_t *args) {
  return NULL; // TODO: implement
}
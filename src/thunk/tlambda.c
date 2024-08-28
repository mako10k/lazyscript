#include "thunk/tlambda.h"
#include "common/malloc.h"
#include <assert.h>

struct lstlambda {
  lstenv_t *ltl_env;
  const lselambda_t *ltl_lambda;
};

lstlambda_t *lstlambda_new(lstenv_t *tenv, const lselambda_t *elambda) {
  assert(tenv != NULL);
  assert(elambda != NULL);
  lstlambda_t *tlambda = lsmalloc(sizeof(lstlambda_t));
  tlambda->ltl_env = lstenv(tenv);
  tlambda->ltl_lambda = elambda;
  return tlambda;
}

#if 0
lsthunk_t *lstlambda_apply(lstlambda_t *tlambda, lsarray_t *args) {
  assert(tlambda != NULL);
  assert(args != NULL);
  lssize_t nlambda_ents = lselambda_get_entry_count(tlambda->elambda);
  lspat_t *param = lsarray_get(args, 0);
  for (lssize_t i = 0; i < nlambda_ents; i++) {
    lselambda_entry_t *lambda_ent = lselambda_get_ent(tlambda->elambda, i);
    lspat_t *arg = lselambda_entry_get_arg(lambda_ent);
    lsexpr_t *body = lselambda_entry_get_body(lambda_ent);
    lstenv_t *tenv = lstenv(tlambda->env);
    if (lspat_match(tenv, param, arg)) {
      lsthunk_subst(body, tenv);
      return lsexpr_thunk(tenv, expr);
    }
  }
  lspat_t * arg = lselambda_get_arg(tlambda->elambda);
  return lselambda_thunk(tlambda->env, tlambda->elambda);
}
#endif
#include "tlambda.h"
#include "expr.h"

struct lstlambda {
  lstenv_t *env;
  const lselambda_t *elambda;
};

lstlambda_t *lstlambda(lstenv_t *tenv, const lselambda_t *elambda) {
  lstlambda_t *tlambda = malloc(sizeof(lstlambda_t));
  tlambda->env = lstenv(tenv);
  tlambda->elambda = elambda;
  return tlambda;
}
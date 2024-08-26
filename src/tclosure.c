#include "tclosure.h"
#include "expr.h"

struct lstclosure {
  lstenv_t *env;
  const lseclosure_t *eclosure;
};

lstclosure_t *lstclosure(lstenv_t *tenv, const lseclosure_t *eclosure) {
  lstclosure_t *tclosure = malloc(sizeof(lstclosure_t));
  tclosure->env = lstenv(tenv);
  tclosure->eclosure = eclosure;
  return tclosure;
}
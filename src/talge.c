#include "talge.h"
#include "ealge.h"

struct lstalge {
  const lsstr_t *constr;
  lsarray_t *args;
};

lstalge_t *lstalge(lstenv_t *tenv, const lsealge_t *ealge) {
  lstalge_t *alge = malloc(sizeof(lstalge_t));
  alge->constr = lsealge_get_constr(ealge);
  alge->args = lsarray();
  for (unsigned int i = 0; i < lsealge_get_argc(ealge); i++) {
    lsexpr_t *arg = lsealge_get_arg(ealge, i);
    lsarray_push(alge->args, lsexpr_thunk(tenv, arg));
  }
  return alge;
}
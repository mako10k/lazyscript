#include "ealge.h"
#include "alge.h"
#include "malloc.h"
#include <assert.h>

struct lsealge {
  lsalge_t *alge;
};

lsealge_t *lsealge(const lsstr_t *constr) {
  assert(constr != NULL);
  lsealge_t *alge = lsmalloc(sizeof(lsealge_t));
  alge->alge = lsalge(constr);
  return alge;
}

void lsealge_push_arg(lsealge_t *alge, lsexpr_t *arg) {
  lsalge_push_arg(alge->alge, arg);
}

void lsealge_push_args(lsealge_t *alge, const lsarray_t *args) {
  lsalge_push_args(alge->alge, args);
}

const lsstr_t *lsealge_get_constr(const lsealge_t *alge) {
  return lsalge_get_constr(alge->alge);
}

unsigned int lsealge_get_argc(const lsealge_t *alge) {
  return lsalge_get_argc(alge->alge);
}

lsexpr_t *lsealge_get_arg(const lsealge_t *alge, int i) {
  return lsalge_get_arg(alge->alge, i);
}

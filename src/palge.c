
#include "palge.h"
#include <assert.h>

struct lspalge {
  const lsstr_t *constr;
  lsarray_t *args;
};

lspalge_t *lspalge(const lsstr_t *constr) {
  assert(constr != NULL);
  lspalge_t *alge = malloc(sizeof(lspalge_t));
  alge->constr = constr;
  alge->args = NULL;
  return alge;
}

void lspalge_push_arg(lspalge_t *alge, lspat_t *arg) {
  assert(alge != NULL);
  assert(arg != NULL);
  if (alge->args == NULL)
    alge->args = lsarray(1);
  lsarray_push(alge->args, arg);
}

void lspalge_push_args(lspalge_t *alge, const lsarray_t *args) {
  assert(alge != NULL);
  alge->args = lsarray_concat(alge->args, args);
}

const lsstr_t *lspalge_get_constr(const lspalge_t *alge) {
  assert(alge != NULL);
  return alge->constr;
}

unsigned int lspalge_get_argc(const lspalge_t *alge) {
  assert(alge != NULL);
  return lsarray_get_size(alge->args);
}

lspat_t *lspalge_get_arg(const lspalge_t *alge, int i) {
  assert(alge != NULL);
  return lsarray_get(alge->args, i);
}

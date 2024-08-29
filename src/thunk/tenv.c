#include "thunk/tenv.h"
#include "common/hash.h"
#include "common/malloc.h"
#include <assert.h>

struct lstenv {
  lshash_t *lte_hash;
  lstenv_t *lte_parent;
};

lstenv_t *lstenv_new(lstenv_t *parent) {
  lstenv_t *tenv = lsmalloc(sizeof(lstenv_t));
  tenv->lte_hash = lshash_new(16);
  tenv->lte_parent = parent;
  return tenv;
}

lsthunk_t *lstenv_get(const lstenv_t *env, const lsstr_t *name) {
  assert(env != NULL);
  assert(name != NULL);
  lsthunk_t *thunk;
  int found = lshash_get(env->lte_hash, name, (lshash_data_t *)&thunk);
  if (found)
    return thunk;
  if (env->lte_parent != NULL)
    return lstenv_get(env->lte_parent, name);
  return NULL;
}

void lstenv_put(lstenv_t *env, const lsstr_t *name, lsthunk_t *thunk) {
  assert(env != NULL);
  assert(name != NULL);
  assert(thunk != NULL);
  lshash_put(env->lte_hash, name, thunk, NULL);
}
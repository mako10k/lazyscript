#include "thunk/tenv.h"
#include "common/hash.h"
#include "common/malloc.h"
#include "thunk/thunk.h"
#include <assert.h>

struct lstenv {
  lshash_t *lte_hash;
  const lstenv_t *lte_parent;
};

lstenv_t *lstenv_new(const lstenv_t *const parent) {
  lstenv_t *const tenv = lsmalloc(sizeof(lstenv_t));
  tenv->lte_hash = lshash_new(16);
  tenv->lte_parent = parent;
  return tenv;
}

lsthunk_t *lstenv_get_self(const lstenv_t *tenv, const lsstr_t *name) {
  lsthunk_t *ret = NULL;
  if (lshash_get(tenv->lte_hash, name, (const void **)&ret))
    return ret;
  return NULL;
}

lsthunk_t *lstenv_get(const lstenv_t *tenv, const lsstr_t *name) {
  lsthunk_t *ret = lstenv_get_self(tenv, name);
  if (ret == NULL && tenv->lte_parent != NULL)
    ret = lstenv_get(tenv->lte_parent, name);
  return ret;
}

void lstenv_put(lstenv_t *tenv, const lsstr_t *name, lsthunk_t *thunk) {
  lsthunk_t *thunk_old;
  lshash_put(tenv->lte_hash, name, thunk, NULL); // TODO: check override?
}

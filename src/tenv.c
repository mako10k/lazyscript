#include "tenv.h"
#include "hash.h"
#include "malloc.h"

struct lstenv {
  lshash_t *lte_hash;
  lstenv_t *lte_parent;
};

lstenv_t *lstenv(lstenv_t *parent) {
  lstenv_t *tenv = lsmalloc(sizeof(lstenv_t));
  tenv->lte_hash = lshash_new(16);
  tenv->lte_parent = parent;
  return tenv;
}
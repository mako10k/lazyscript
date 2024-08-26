#include "tenv.h"
#include "hash.h"

struct lstenv {
  lshash_t *hash;
  lstenv_t *parent;
};

lstenv_t *lstenv(lstenv_t *parent) {
  lstenv_t *tenv = malloc(sizeof(lstenv_t));
  tenv->hash = lshash(16);
  tenv->parent = parent;
  return tenv;
}
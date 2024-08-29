#include "thunk/tenv.h"
#include "common/hash.h"
#include "common/malloc.h"
#include <assert.h>

typedef struct lscounter {
  lssize_t lc_nwarn;
  lssize_t lc_nerror;
  lssize_t lc_nfatal;
} lscounter_t;

struct lstenv {
  lshash_t *lee_refs;
  lscounter_t *lee_counter;
  const lstenv_t *lee_parent;
};

lstenv_t *lstenv_new(const lstenv_t *const parent) {
  lstenv_t *const tenv = lsmalloc(sizeof(lstenv_t));
  tenv->lee_refs = lshash_new(16);
  if (parent != NULL)
    tenv->lee_counter = parent->lee_counter;
  else {
    tenv->lee_counter = lsmalloc(sizeof(lscounter_t));
    tenv->lee_counter->lc_nwarn = 0;
    tenv->lee_counter->lc_nerror = 0;
    tenv->lee_counter->lc_nfatal = 0;
  }
  tenv->lee_parent = parent;
  return tenv;
}

lstref_t *lstenv_get(const lstenv_t *tenv, const lsstr_t *name) {
  assert(tenv != NULL);
  assert(name != NULL);
  lstref_t *tref;
  int found = lshash_get(tenv->lee_refs, name, (lshash_data_t *)&tref);
  if (found)
    return tref;
  if (tenv->lee_parent != NULL)
    return lstenv_get(tenv->lee_parent, name);
  return NULL;
}

lstref_t *lstenv_get_self(const lstenv_t *tenv, const lsstr_t *name) {
  assert(tenv != NULL);
  assert(name != NULL);
  lstref_t *tref;
  int found = lshash_get(tenv->lee_refs, name, (lshash_data_t *)&tref);
  if (found)
    return tref;
  return NULL;
}

void lstenv_put(lstenv_t *tenv, const lsstr_t *name, const lstref_t *tref) {
  assert(tenv != NULL);
  assert(name != NULL);
  assert(tref != NULL);
  lshash_put(tenv->lee_refs, name, tref, NULL);
}

void lstenv_incr_nwarnings(lstenv_t *tenv) {
  assert(tenv != NULL);
  tenv->lee_counter->lc_nwarn++;
}

void lstenv_incr_nerrors(lstenv_t *tenv) {
  assert(tenv != NULL);
  tenv->lee_counter->lc_nerror++;
}

void lstenv_incr_nfatals(lstenv_t *tenv) {
  assert(tenv != NULL);
  tenv->lee_counter->lc_nfatal++;
}

lssize_t lstenv_get_nwarnings(const lstenv_t *tenv) {
  assert(tenv != NULL);
  return tenv->lee_counter->lc_nwarn;
}

lssize_t lstenv_get_nerrors(const lstenv_t *tenv) {
  assert(tenv != NULL);
  return tenv->lee_counter->lc_nerror;
}

lssize_t lstenv_get_nfatals(const lstenv_t *tenv) {
  assert(tenv != NULL);
  return tenv->lee_counter->lc_nfatal;
}

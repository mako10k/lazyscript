#include "expr/eenv.h"
#include "common/hash.h"
#include "common/malloc.h"
#include <assert.h>

typedef struct lscounter {
  lssize_t lc_nwarn;
  lssize_t lc_nerror;
  lssize_t lc_nfatal;
} lscounter_t;

struct lseenv {
  lshash_t *lee_refs;
  lscounter_t *lee_counter;
  const lseenv_t *lee_parent;
};

lseenv_t *lseenv_new(const lseenv_t *const parent) {
  lseenv_t *const eenv = lsmalloc(sizeof(lseenv_t));
  eenv->lee_refs = lshash_new(16);
  if (parent != NULL)
    eenv->lee_counter = parent->lee_counter;
  else {
    eenv->lee_counter = lsmalloc(sizeof(lscounter_t));
    eenv->lee_counter->lc_nwarn = 0;
    eenv->lee_counter->lc_nerror = 0;
    eenv->lee_counter->lc_nfatal = 0;
  }
  eenv->lee_parent = parent;
  return eenv;
}

lserref_t *lseenv_get(const lseenv_t *eenv, const lsstr_t *name) {
  assert(eenv != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(eenv->lee_refs, name, (lshash_data_t *)&erref);
  if (found)
    return erref;
  if (eenv->lee_parent != NULL)
    return lseenv_get(eenv->lee_parent, name);
  return NULL;
}

lserref_t *lseenv_get_self(const lseenv_t *eenv, const lsstr_t *name) {
  assert(eenv != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(eenv->lee_refs, name, (lshash_data_t *)&erref);
  if (found)
    return erref;
  return NULL;
}

void lseenv_put(lseenv_t *eenv, const lsstr_t *name, const lserref_t *erref) {
  assert(eenv != NULL);
  assert(name != NULL);
  assert(erref != NULL);
  lshash_put(eenv->lee_refs, name, erref, NULL);
}

void lseenv_incr_nwarnings(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->lee_counter->lc_nwarn++;
}

void lseenv_incr_nerrors(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->lee_counter->lc_nerror++;
}

void lseenv_incr_nfatals(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->lee_counter->lc_nfatal++;
}

lssize_t lseenv_get_nwarnings(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->lee_counter->lc_nwarn;
}

lssize_t lseenv_get_nerrors(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->lee_counter->lc_nerror;
}

lssize_t lseenv_get_nfatals(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->lee_counter->lc_nfatal;
}

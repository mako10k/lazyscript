#include "expr/eenv.h"
#include "common/hash.h"
#include "common/malloc.h"
#include <assert.h>

struct lseenv {
  lshash_t *lee_refs;
  lssize_t lee_nwarn;
  lssize_t lee_nerror;
  lssize_t lee_nfatal;
  lseenv_t *lee_parent;
};

lseenv_t *lseenv_new(lseenv_t *const parent) {
  lseenv_t *const eenv = lsmalloc(sizeof(lseenv_t));
  eenv->lee_refs = lshash_new(16);
  eenv->lee_nwarn = 0;
  eenv->lee_nerror = 0;
  eenv->lee_nfatal = 0;
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
  eenv->lee_nwarn++;
  if (eenv->lee_parent != NULL)
    lseenv_incr_nwarnings(eenv->lee_parent);
}

void lseenv_incr_nerrors(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->lee_nerror++;
  if (eenv->lee_parent != NULL)
    lseenv_incr_nerrors(eenv->lee_parent);
}

void lseenv_incr_nfatals(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->lee_nfatal++;
  if (eenv->lee_parent != NULL)
    lseenv_incr_nfatals(eenv->lee_parent);
}

lssize_t lseenv_get_nwarnings(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->lee_nwarn;
}

lssize_t lseenv_get_nerrors(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->lee_nerror;
}

lssize_t lseenv_get_nfatals(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->lee_nfatal;
}

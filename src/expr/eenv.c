#include "eenv.h"
#include "hash.h"
#include "malloc.h"
#include <assert.h>

struct lseenv {
  lshash_t *le_refs;
  lssize_t le_nwarn;
  lssize_t le_nerror;
  lssize_t le_nfatal;
  lseenv_t *le_parent;
};

lseenv_t *lseenv_new(lseenv_t *const parent) {
  lseenv_t *const eenv = lsmalloc(sizeof(lseenv_t));
  eenv->le_refs = lshash_new(16);
  eenv->le_nwarn = 0;
  eenv->le_nerror = 0;
  eenv->le_nfatal = 0;
  eenv->le_parent = parent;
  return eenv;
}

lserref_t *lseenv_get(const lseenv_t *eenv, const lsstr_t *name) {
  assert(eenv != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(eenv->le_refs, name, (void **)&erref);
  if (found)
    return erref;
  if (eenv->le_parent != NULL)
    return lseenv_get(eenv->le_parent, name);
  return NULL;
}

lserref_t *lseenv_get_self(const lseenv_t *eenv, const lsstr_t *name) {
  assert(eenv != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(eenv->le_refs, name, (void **)&erref);
  if (found)
    return erref;
  return NULL;
}

void lseenv_put(lseenv_t *eenv, const lsstr_t *name, lserref_t *erref) {
  assert(eenv != NULL);
  assert(name != NULL);
  assert(erref != NULL);
  lshash_put(eenv->le_refs, name, erref, NULL);
}

void lseenv_incr_nwarnings(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->le_nwarn++;
  if (eenv->le_parent != NULL)
    lseenv_incr_nwarnings(eenv->le_parent);
}

void lseenv_incr_nerrors(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->le_nerror++;
  if (eenv->le_parent != NULL)
    lseenv_incr_nerrors(eenv->le_parent);
}

void lseenv_incr_nfatals(lseenv_t *eenv) {
  assert(eenv != NULL);
  eenv->le_nfatal++;
  if (eenv->le_parent != NULL)
    lseenv_incr_nfatals(eenv->le_parent);
}

lssize_t lseenv_get_nwarnings(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->le_nwarn;
}

lssize_t lseenv_get_nerrors(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->le_nerror;
}

lssize_t lseenv_get_nfatals(const lseenv_t *eenv) {
  assert(eenv != NULL);
  return eenv->le_nfatal;
}

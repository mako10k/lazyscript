#include "eenv.h"
#include "hash.h"
#include <assert.h>

struct lseenv {
  lshash_t *refs;
  unsigned int nwarn;
  unsigned int nerror;
  unsigned int nfatal;
  lseenv_t *parent;
};

lseenv_t *lseenv(lseenv_t *const parent) {
  lseenv_t *const env = malloc(sizeof(lseenv_t));
  env->refs = lshash(16);
  env->nwarn = 0;
  env->nerror = 0;
  env->nfatal = 0;
  env->parent = parent;
  return env;
}

lserref_t *lseenv_get(const lseenv_t *env, const lsstr_t *name) {
  assert(env != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(env->refs, name, (void **)&erref);
  if (found)
    return erref;
  if (env->parent != NULL)
    return lseenv_get(env->parent, name);
  return NULL;
}

lserref_t *lseenv_get_self(const lseenv_t *env, const lsstr_t *name) {
  assert(env != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(env->refs, name, (void **)&erref);
  if (found)
    return erref;
  return NULL;
}

void lseenv_put(lseenv_t *env, const lsstr_t *name, lserref_t *erref) {
  assert(env != NULL);
  assert(name != NULL);
  assert(erref != NULL);
  lshash_put(env->refs, name, erref, NULL);
}

void lseenv_incr_nwarnings(lseenv_t *env) {
  assert(env != NULL);
  env->nwarn++;
  if (env->parent != NULL)
    lseenv_incr_nwarnings(env->parent);
}

void lseenv_incr_nerrors(lseenv_t *env) {
  assert(env != NULL);
  env->nerror++;
  if (env->parent != NULL)
    lseenv_incr_nerrors(env->parent);
}

void lseenv_incr_nfatals(lseenv_t *env) {
  assert(env != NULL);
  env->nfatal++;
  if (env->parent != NULL)
    lseenv_incr_nfatals(env->parent);
}

int lseenv_get_nwarnings(const lseenv_t *env) {
  assert(env != NULL);
  return env->nwarn;
}

int lseenv_get_nerrors(const lseenv_t *env) {
  assert(env != NULL);
  return env->nerror;
}

int lseenv_get_nfatals(const lseenv_t *env) {
  assert(env != NULL);
  return env->nfatal;
}

#include "env.h"
#include "hash.h"
#include <assert.h>

struct lsenv {
  lshash_t *refs;
  unsigned int nwarn;
  unsigned int nerror;
  unsigned int nfatal;
  lsenv_t *parent;
};

lsenv_t *lsenv(lsenv_t *const parent) {
  lsenv_t *const env = malloc(sizeof(lsenv_t));
  env->refs = lshash(16);
  env->nwarn = 0;
  env->nerror = 0;
  env->nfatal = 0;
  env->parent = parent;
  return env;
}

lserref_t *lsenv_get(const lsenv_t *env, const lsstr_t *name) {
  assert(env != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(env->refs, name, (void **)&erref);
  if (found)
    return erref;
  if (env->parent != NULL)
    return lsenv_get(env->parent, name);
  return NULL;
}

lserref_t *lsenv_get_self(const lsenv_t *env, const lsstr_t *name) {
  assert(env != NULL);
  assert(name != NULL);
  lserref_t *erref;
  int found = lshash_get(env->refs, name, (void **)&erref);
  if (found)
    return erref;
  return NULL;
}

void lsenv_put(lsenv_t *env, const lsstr_t *name, lserref_t *erref) {
  assert(env != NULL);
  assert(name != NULL);
  assert(erref != NULL);
  lshash_put(env->refs, name, erref, NULL);
}

void lsenv_incr_nwarnings(lsenv_t *env) {
  assert(env != NULL);
  env->nwarn++;
  if (env->parent != NULL)
    lsenv_incr_nwarnings(env->parent);
}

void lsenv_incr_nerrors(lsenv_t *env) {
  assert(env != NULL);
  env->nerror++;
  if (env->parent != NULL)
    lsenv_incr_nerrors(env->parent);
}

void lsenv_incr_nfatals(lsenv_t *env) {
  assert(env != NULL);
  env->nfatal++;
  if (env->parent != NULL)
    lsenv_incr_nfatals(env->parent);
}

int lsenv_get_nwarnings(const lsenv_t *env) {
  assert(env != NULL);
  return env->nwarn;
}

int lsenv_get_nerrors(const lsenv_t *env) {
  assert(env != NULL);
  return env->nerror;
}

int lsenv_get_nfatals(const lsenv_t *env) {
  assert(env != NULL);
  return env->nfatal;
}

#include "env.h"
#include "hash.h"
#include <assert.h>

struct lsenv {
  lshash_t *refs;
  const lsenv_t *parent;
};

lsenv_t *lsenv(const lsenv_t *const parent) {
  lsenv_t *const env = malloc(sizeof(lsenv_t));
  env->refs = lshash(16);
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

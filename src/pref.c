#include "pref.h"
#include "io.h"
#include "malloc.h"
#include <assert.h>
#include <stdio.h>

struct lspref {
  const lsstr_t *name;
  lsloc_t loc;
};

lspref_t *lspref(const lsstr_t *name, lsloc_t loc) {
  assert(name != NULL);
  lspref_t *pref = lsmalloc(sizeof(lspref_t));
  pref->name = name;
  pref->loc = loc;
  return pref;
}

const lsstr_t *lspref_get_name(const lspref_t *ref) {
  assert(ref != NULL);
  return ref->name;
}

lsloc_t lspref_get_loc(const lspref_t *ref) {
  assert(ref != NULL);
  return ref->loc;
}

void lspref_print(FILE *fp, int prec, int indent, const lspref_t *ref) {
  assert(fp != NULL);
  assert(ref != NULL);
  (void)prec;
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, ref->name);
}

int lspref_prepare(lspref_t *ref, lseenv_t *env, lserref_wrapper_t *erref) {
  assert(ref != NULL);
  assert(env != NULL);
  assert(erref != NULL);
  lserref_t *erref_found = lseenv_get_self(env, ref->name);
  if (erref_found != NULL) {
    lspref_t *pref_found = lserref_get_pref(erref_found);
    lsprintf(stderr, 0, "E: ");
    lsloc_print(stderr, ref->loc);
    lsprintf(stderr, 0, "duplicated reference: ");
    lspref_print(stderr, 0, 0, ref);
    lsprintf(stderr, 0, " (");
    lsloc_print(stderr, lspref_get_loc(pref_found));
    lsprintf(stderr, 0, "former reference: ");
    lspref_print(stderr, 0, 0, pref_found);
    lsprintf(stderr, 0, ")\n");
    lseenv_incr_nerrors(env);
    return 0;
  }
  erref_found = lseenv_get(env, ref->name);
  if (erref_found != NULL) {
    lspref_t *pref_found = lserref_get_pref(erref_found);
    lsprintf(stderr, 0, "W: ");
    lsloc_print(stderr, ref->loc);
    lsprintf(stderr, 0, "overridden reference: ");
    lspref_print(stderr, 0, 0, ref);
    lsprintf(stderr, 0, " (");
    lsloc_print(stderr, lspref_get_loc(pref_found));
    lsprintf(stderr, 0, "former reference: ");
    lspref_print(stderr, 0, 0, pref_found);
    lsprintf(stderr, 0, ")\n");
    lseenv_incr_nwarnings(env);
    return 0;
  }
  lseenv_put(env, ref->name, lserref(erref, ref));
  return 0;
}
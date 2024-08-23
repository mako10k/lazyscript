#include "pref.h"
#include "lazyscript.h"
#include "malloc.h"

struct lspref {
  const lsstr_t *name;
};

lspref_t *lspref(const lsstr_t *name) {
  lspref_t *pref = lsmalloc(sizeof(lspref_t));
  pref->name = name;
  return pref;
}

const lsstr_t *lspref_get_name(const lspref_t *ref) { return ref->name; }

void lspref_print(FILE *fp, int prec, int indent, const lspref_t *ref) {
  (void)prec;
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, ref->name);
}

int lspref_prepare(lspref_t *ref, lsenv_t *env, lserref_t *erref) {
  if (lsenv_get_self(env, ref->name) != NULL) {
    lsprintf(stderr, 0, "error: reference already defined: ");
    lspref_print(stderr, 0, 0, ref);
    lsprintf(stderr, 0, "\n");
    return -1;
  }
  lsenv_put(env, ref->name, erref);
  return 1;
}
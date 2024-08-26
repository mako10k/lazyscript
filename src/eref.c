#include "eref.h"
#include "erref.h"
#include "io.h"
#include "malloc.h"
#include "str.h"
#include <assert.h>

struct lseref {
  const lsstr_t *name;
  lserref_t *erref;
  lsloc_t loc;
};

lseref_t *lseref(const lsstr_t *name, lsloc_t loc) {
  assert(name != NULL);
  assert(loc.filename != NULL);
  lseref_t *eref = lsmalloc(sizeof(lseref_t));
  eref->name = name;
  eref->erref = NULL;
  eref->loc = loc;
  return eref;
}

const lsstr_t *lseref_get_name(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->name;
}

void lseref_set_erref(lseref_t *eref, lserref_t *erref) {
  assert(eref != NULL);
  eref->erref = erref;
}

lserref_t *lseref_get_erref(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->erref;
}

lsloc_t lseref_get_loc(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->loc;
}

void lseref_print(FILE *fp, int prec, int indent, const lseref_t *eref) {
  assert(fp != NULL);
  assert(eref != NULL);
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, eref->name);
}

int lseref_prepare(lseref_t *eref, lseenv_t *env) {
  assert(eref != NULL);
  assert(env != NULL);
  if (lseenv_get(env, eref->name) != NULL)
    return 0;
  lsprintf(stderr, 0, "E: ");
  lsloc_print(stderr, lseref_get_loc(eref));
  lsprintf(stderr, 0, "undefined reference: ");
  lseref_print(stderr, 0, 0, eref);
  lsprintf(stderr, 0, "\n");
  lseenv_incr_nerrors(env);
  return 0;
}
#include "eref.h"
#include "erref.h"
#include "lazyscript.h"
#include "malloc.h"
#include "str.h"
#include <assert.h>

struct lseref {
  const lsstr_t *name;
  lserref_t *erref;
};

lseref_t *lseref(const lsstr_t *name) {
  assert(name != NULL);
  lseref_t *eref = lsmalloc(sizeof(lseref_t));
  eref->name = name;
  eref->erref = NULL;
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

void lseref_print(FILE *fp, int prec, int indent, const lseref_t *eref) {
  assert(fp != NULL);
  assert(eref != NULL);
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, eref->name);
}

int lseref_prepare(lseref_t *eref, lsenv_t *env) {
  assert(eref != NULL);
  assert(env != NULL);
  if (lsenv_get(env, eref->name) != NULL)
    return 0;
  lsprintf(stderr, 0, "error: undefined reference: ");
  lseref_print(stderr, 0, 0, eref);
  lsprintf(stderr, 0, "\n");
  return -1;
}
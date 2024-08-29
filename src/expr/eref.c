#include "expr/eref.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "expr/erref.h"
#include "lstypes.h"
#include <assert.h>

struct lseref {
  const lsstr_t *ler_name;
  lserref_t *ler_erref;
  lsloc_t ler_loc;
};

lseref_t *lseref_new(const lsstr_t *name, lsloc_t loc) {
  assert(name != NULL);
  lseref_t *eref = lsmalloc(sizeof(lseref_t));
  eref->ler_name = name;
  eref->ler_erref = NULL;
  eref->ler_loc = loc;
  return eref;
}

const lsstr_t *lseref_get_name(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->ler_name;
}

void lseref_set_erref(lseref_t *eref, lserref_t *erref) {
  assert(eref != NULL);
  eref->ler_erref = erref;
}

lserref_t *lseref_get_erref(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->ler_erref;
}

lsloc_t lseref_get_loc(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->ler_loc;
}

void lseref_print(FILE *fp, lsprec_t prec, int indent, const lseref_t *eref) {
  assert(fp != NULL);
  assert(eref != NULL);
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, eref->ler_name);
}

lspres_t lseref_prepare(lseref_t *eref, lseenv_t *env) {
  assert(eref != NULL);
  assert(env != NULL);
  lserref_t *erref = lseenv_get(env, eref->ler_name);
  if (erref != NULL)
    return LSPRES_SUCCESS;
  lsprintf(stderr, 0, "E: ");
  lsloc_print(stderr, lseref_get_loc(eref));
  lsprintf(stderr, 0, "undefined reference: ");
  lseref_print(stderr, 0, 0, eref);
  lsprintf(stderr, 0, "\n");
  lseenv_incr_nerrors(env);
  return LSPRES_SUCCESS;
}
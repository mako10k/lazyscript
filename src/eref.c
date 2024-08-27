#include "eref.h"
#include "erref.h"
#include "io.h"
#include "lstypes.h"
#include "malloc.h"
#include "str.h"
#include <assert.h>

struct lseref {
  const lsstr_t *lerf_name;
  lserref_t *lerf_erref;
  lsloc_t lerf_loc;
};

lseref_t *lseref_new(const lsstr_t *name, lsloc_t loc) {
  assert(name != NULL);
  assert(loc.filename != NULL);
  lseref_t *eref = lsmalloc(sizeof(lseref_t));
  eref->lerf_name = name;
  eref->lerf_erref = NULL;
  eref->lerf_loc = loc;
  return eref;
}

const lsstr_t *lseref_get_name(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->lerf_name;
}

void lseref_set_erref(lseref_t *eref, lserref_t *erref) {
  assert(eref != NULL);
  eref->lerf_erref = erref;
}

lserref_t *lseref_get_erref(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->lerf_erref;
}

lsloc_t lseref_get_loc(const lseref_t *eref) {
  assert(eref != NULL);
  return eref->lerf_loc;
}

void lseref_print(FILE *fp, lsprec_t prec, int indent, const lseref_t *eref) {
  assert(fp != NULL);
  assert(eref != NULL);
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, eref->lerf_name);
}

lspres_t lseref_prepare(lseref_t *eref, lseenv_t *env) {
  assert(eref != NULL);
  assert(env != NULL);
  lserref_t *erref = lseenv_get(env, eref->lerf_name);
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
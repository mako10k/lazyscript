#include "pref.h"
#include "io.h"
#include "lstypes.h"
#include "malloc.h"
#include <assert.h>
#include <stdio.h>

struct lspref {
  const lsstr_t *lpr_name;
  lsloc_t lpr_loc;
};

lspref_t *lspref_new(const lsstr_t *name, lsloc_t loc) {
  assert(name != NULL);
  lspref_t *pref = lsmalloc(sizeof(lspref_t));
  pref->lpr_name = name;
  pref->lpr_loc = loc;
  return pref;
}

const lsstr_t *lspref_get_name(const lspref_t *pref) {
  assert(pref != NULL);
  return pref->lpr_name;
}

lsloc_t lspref_get_loc(const lspref_t *pref) {
  assert(pref != NULL);
  return pref->lpr_loc;
}

void lspref_print(FILE *fp, lsprec_t prec, int indent, const lspref_t *pref) {
  assert(fp != NULL);
  assert(pref != NULL);
  (void)prec;
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, pref->lpr_name);
}

lspres_t lspref_prepare(lspref_t *pref, lseenv_t *eenv,
                        lserref_wrapper_t *erref) {
  assert(pref != NULL);
  assert(eenv != NULL);
  assert(erref != NULL);
  lserref_t *erref_found = lseenv_get_self(eenv, pref->lpr_name);
  if (erref_found != NULL) {
    lspref_t *pref_found = lserref_get_pref(erref_found);
    lsprintf(stderr, 0, "E: ");
    lsloc_print(stderr, pref->lpr_loc);
    lsprintf(stderr, 0, "duplicated reference: ");
    lspref_print(stderr, 0, 0, pref);
    lsprintf(stderr, 0, " (");
    lsloc_print(stderr, lspref_get_loc(pref_found));
    lsprintf(stderr, 0, "former reference: ");
    lspref_print(stderr, 0, 0, pref_found);
    lsprintf(stderr, 0, ")\n");
    lseenv_incr_nerrors(eenv);
    return LSPRES_SUCCESS;
  }
  erref_found = lseenv_get(eenv, pref->lpr_name);
  if (erref_found != NULL) {
    lspref_t *pref_found = lserref_get_pref(erref_found);
    lsprintf(stderr, 0, "W: ");
    lsloc_print(stderr, pref->lpr_loc);
    lsprintf(stderr, 0, "overridden reference: ");
    lspref_print(stderr, 0, 0, pref);
    lsprintf(stderr, 0, " (");
    lsloc_print(stderr, lspref_get_loc(pref_found));
    lsprintf(stderr, 0, "former reference: ");
    lspref_print(stderr, 0, 0, pref_found);
    lsprintf(stderr, 0, ")\n");
    lseenv_incr_nwarnings(eenv);
    return LSPRES_SUCCESS;
  }
  lseenv_put(eenv, pref->lpr_name, lserref_new(erref, pref));
  return LSPRES_SUCCESS;
}
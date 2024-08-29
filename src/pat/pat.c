#include "pat/pat.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "lstypes.h"
#include <assert.h>

struct lspat {
  lsptype_t lp_type;
  union {
    const lspalge_t *lp_alge;
    const lspas_t *lp_as;
    const lsint_t *lp_int;
    const lsstr_t *lp_str;
    const lsref_t *lp_ref;
  };
};

const lspat_t *lspat_new_alge(const lspalge_t *palge) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_ALGE;
  pat->lp_alge = palge;
  return pat;
}

const lspat_t *lspat_new_as(const lspas_t *pas) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_AS;
  pat->lp_as = pas;
  return pat;
}

const lspat_t *lspat_new_int(const lsint_t *val) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_INT;
  pat->lp_int = val;
  return pat;
}

const lspat_t *lspat_new_str(const lsstr_t *val) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_STR;
  pat->lp_str = val;
  return pat;
}

lsptype_t lspat_get_type(const lspat_t *pat) { return pat->lp_type; }

const lspalge_t *lspat_get_alge(const lspat_t *pat) { return pat->lp_alge; }

const lspas_t *lspat_get_as(const lspat_t *pat) { return pat->lp_as; }

const lsint_t *lspat_get_int(const lspat_t *pat) { return pat->lp_int; }

const lsstr_t *lspat_get_str(const lspat_t *pat) { return pat->lp_str; }

const lsref_t *lspat_get_ref(const lspat_t *pat) { return pat->lp_ref; }

void lspat_print(FILE *fp, lsprec_t prec, int indent, const lspat_t *pat) {
  switch (pat->lp_type) {
  case LSPTYPE_ALGE:
    lspalge_print(fp, prec, indent, pat->lp_alge);
    break;
  case LSPTYPE_AS:
    lspas_print(fp, prec, indent, pat->lp_as);
    break;
  case LSPTYPE_INT:
    lsint_print(fp, prec, indent, pat->lp_int);
    break;
  case LSPTYPE_STR:
    lsstr_print(fp, prec, indent, pat->lp_str);
    break;
  case LSPTYPE_REF:
    lsref_print(fp, prec, indent, pat->lp_ref);
    break;
  }
}

const lspat_t *lspat_new_ref(const lsref_t *ref) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_REF;
  pat->lp_ref = ref;
  return pat;
}

#include "pat/pat.h"
#include "common/malloc.h"
#include "lstypes.h"
#include <assert.h>

struct lspat {
  lsptype_t lp_type;
  union {
    lspalge_t *lp_alge;
    lspas_t *lp_as;
    const lsint_t *lp_int;
    const lsstr_t *lp_str;
    lspref_t *lp_ref;
  };
};

lspat_t *lspat_new_alge(lspalge_t *palge) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_ALGE;
  pat->lp_alge = palge;
  return pat;
}

lspat_t *lspat_new_as(lspas_t *pas) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_AS;
  pat->lp_as = pas;
  return pat;
}

lspat_t *lspat_new_int(const lsint_t *val) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_INT;
  pat->lp_int = val;
  return pat;
}

lspat_t *lspat_new_str(const lsstr_t *val) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_STR;
  pat->lp_str = val;
  return pat;
}

lsptype_t lspat_get_type(const lspat_t *pat) { return pat->lp_type; }

lspalge_t *lspat_get_alge(const lspat_t *pat) { return pat->lp_alge; }

lspas_t *lspat_get_as(const lspat_t *pat) { return pat->lp_as; }

const lsint_t *lspat_get_int(const lspat_t *pat) { return pat->lp_int; }

const lsstr_t *lspat_get_str(const lspat_t *pat) { return pat->lp_str; }

lspref_t *lspat_get_ref(const lspat_t *pat) { return pat->lp_ref; }

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
    lspref_print(fp, prec, indent, pat->lp_ref);
    break;
  }
  assert(0);
}

lspat_t *lspat_new_ref(lspref_t *pref) {
  lspat_t *pat = lsmalloc(sizeof(lspat_t));
  pat->lp_type = LSPTYPE_REF;
  pat->lp_ref = pref;
  return pat;
}

lspres_t lspat_prepare(lspat_t *pat, lseenv_t *env, lserref_wrapper_t *erref) {
  switch (pat->lp_type) {
  case LSPTYPE_ALGE:
    return lspalge_prepare(pat->lp_alge, env, erref);
  case LSPTYPE_AS:
    return lspas_prepare(pat->lp_as, env, erref);
  case LSPTYPE_INT:
    return LSPRES_SUCCESS;
  case LSPTYPE_STR:
    return LSPRES_SUCCESS;
  case LSPTYPE_REF:
    return lspref_prepare(pat->lp_ref, env, erref);
  }
  return LSPRES_SUCCESS;
}
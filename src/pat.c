#include "pat.h"

struct lspat {
  lsptype_t type;
  union {
    lspalge_t *alge;
    lsas_t *as;
    const lsint_t *intval;
    const lsstr_t *strval;
    const lspref_t *pref;
  };
};

lspat_t *lspat_alge(lspalge_t *alge) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_ALGE;
  pat->alge = alge;
  return pat;
}

lspat_t *lspat_as(lsas_t *as) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_AS;
  pat->as = as;
  return pat;
}

lspat_t *lspat_int(const lsint_t *intval) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_INT;
  pat->intval = intval;
  return pat;
}

lspat_t *lspat_str(const lsstr_t *strval) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_STR;
  pat->strval = strval;
  return pat;
}

void lspat_print(FILE *fp, int prec, int indent, const lspat_t *pat) {
  switch (pat->type) {
  case LSPTYPE_ALGE:
    lspalge_print(fp, prec, indent, pat->alge);
    break;
  case LSPTYPE_AS:
    lsas_print(fp, prec, indent, pat->as);
    break;
  case LSPTYPE_INT:
    lsint_print(fp, prec, indent, pat->intval);
    break;
  case LSPTYPE_STR:
    lsstr_print(fp, prec, indent, pat->strval);
    break;
  case LSPTYPE_REF:
    lspref_print(fp, prec, indent, pat->pref);
    break;
  }
}

lspat_t *lspat_ref(const lspref_t *pref) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_REF;
  pat->pref = pref;
  return pat;
}
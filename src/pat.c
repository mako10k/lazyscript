#include "pat.h"

struct lspat {
  lsptype_t type;
  union {
    lspalge_t *alge;
    lsas_t *as;
    const lsint_t *intval;
    const lsstr_t *strval;
    lspref_t *pref;
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

lspat_t *lspat_ref(lspref_t *pref) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_REF;
  pat->pref = pref;
  return pat;
}

int lspat_prepare(lspat_t *pat, lsenv_t *env, lserref_t *erref) {
  switch (pat->type) {
  case LSPTYPE_ALGE:
    return lspalge_prepare(pat->alge, env, erref);
  case LSPTYPE_AS:
    return lsas_prepare(pat->as, env, erref);
  case LSPTYPE_INT:
    return 1;
  case LSPTYPE_STR:
    return 1;
  case LSPTYPE_REF:
    return lspref_prepare(pat->pref, env, erref);
  }
  return 0;
}
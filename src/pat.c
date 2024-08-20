#include "pat.h"

struct lspat {
  lsptype_t type;
  union {
    lspalge_t *alge;
    lsas_t *as;
    const lsint_t *intval;
    const lsstr_t *strval;
  } value;
};

lspat_t *lspat_alge(lspalge_t *alge) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_ALGE;
  pat->value.alge = alge;
  return pat;
}

lspat_t *lspat_as(lsas_t *as) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_AS;
  pat->value.as = as;
  return pat;
}

lspat_t *lspat_int(const lsint_t *intval) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_INT;
  pat->value.intval = intval;
  return pat;
}

lspat_t *lspat_str(const lsstr_t *strval) {
  lspat_t *pat = malloc(sizeof(lspat_t));
  pat->type = LSPTYPE_STR;
  pat->value.strval = strval;
  return pat;
}

void lspat_print(FILE *fp, int prec, const lspat_t *pat) {
  switch (pat->type) {
  case LSPTYPE_ALGE:
    lspalge_print(fp, prec, pat->value.alge);
    break;
  case LSPTYPE_AS:
    lsas_print(fp, prec, pat->value.as);
    break;
  case LSPTYPE_INT:
    lsint_print(fp, pat->value.intval);
    break;
  case LSPTYPE_STR:
    lsstr_print(fp, pat->value.strval);
    break;
  }
}
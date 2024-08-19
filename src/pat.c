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

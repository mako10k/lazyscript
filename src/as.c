#include "as.h"
#include "lazyscript.h"

struct lsas {
  lspref_t *pref;
  const lspat_t *pat;
};

lsas_t *lsas(lspref_t *pref, const lspat_t *pat) {
  lsas_t *as = malloc(sizeof(lsas_t));
  as->pref = pref;
  as->pat = pat;
  return as;
}

void lsas_print(FILE *fp, int prec, int indent, const lsas_t *as) {
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lspref_print(fp, prec, indent, as->pref);
  lsprintf(fp, indent, "@");
  lspat_print(fp, LSPREC_APPL + 1, indent, as->pat);
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}
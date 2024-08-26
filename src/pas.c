#include "pas.h"
#include "io.h"
#include "lazyscript.h"

struct lspas {
  lspref_t *pref;
  lspat_t *pat;
};

lspas_t *lspas(lspref_t *pref, lspat_t *pat) {
  lspas_t *as = malloc(sizeof(lspas_t));
  as->pref = pref;
  as->pat = pat;
  return as;
}

void lspas_print(FILE *fp, int prec, int indent, lspas_t *as) {
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lspref_print(fp, prec, indent, as->pref);
  lsprintf(fp, indent, "@");
  lspat_print(fp, LSPREC_APPL + 1, indent, as->pat);
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

int lspas_prepare(lspas_t *as, lseenv_t *env, lserref_wrapper_t *erref) {
  int res = lspat_prepare(as->pat, env, erref);
  if (res < 0)
    return res;
  res = lspref_prepare(as->pref, env, erref);
  if (res < 0)
    return res;
  return 0;
}
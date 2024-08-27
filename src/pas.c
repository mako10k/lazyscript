#include "pas.h"
#include "io.h"
#include "lstypes.h"
#include "malloc.h"

struct lspas {
  lspref_t *lpa_pref;
  lspat_t *lpa_pat;
};

lspas_t *lspas_new(lspref_t *pref, lspat_t *pat) {
  lspas_t *pas = lsmalloc(sizeof(lspas_t));
  pas->lpa_pref = pref;
  pas->lpa_pat = pat;
  return pas;
}

void lspas_print(FILE *fp, int prec, int indent, lspas_t *pas) {
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lspref_print(fp, prec, indent, pas->lpa_pref);
  lsprintf(fp, indent, "@");
  lspat_print(fp, LSPREC_APPL + 1, indent, pas->lpa_pat);
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

lspres_t lspas_prepare(lspas_t *pas, lseenv_t *env, lserref_wrapper_t *erref) {
  lspres_t res = lspat_prepare(pas->lpa_pat, env, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  res = lspref_prepare(pas->lpa_pref, env, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  return LSPRES_SUCCESS;
}

lsmres_t lspas_match(lstenv_t *tenv, const lspas_t *pas, lsthunk_t *thunk) {
  lsmres_t mres = lspat_match(tenv, pas->lpa_pat, thunk);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  mres = lspref_match(tenv, pas->lpa_pref, thunk);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return LSMATCH_SUCCESS;
}
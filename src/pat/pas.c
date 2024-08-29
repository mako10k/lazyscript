#include "pat/pas.h"
#include "common/io.h"
#include "common/malloc.h"
#include "lstypes.h"
#include <assert.h>

struct lspas {
  const lspref_t *lpa_pref;
  const lspat_t *lpa_pat;
};

const lspas_t *lspas_new(const lspref_t *pref, const lspat_t *pat) {
  assert(pref != NULL);
  assert(pat != NULL);
  lspas_t *pas = lsmalloc(sizeof(lspas_t));
  pas->lpa_pref = pref;
  pas->lpa_pat = pat;
  return pas;
}

void lspas_print(FILE *fp, lsprec_t prec, int indent, const lspas_t *pas) {
  assert(fp != NULL);
  assert(LSPREC_LOWEST <= prec && prec <= LSPREC_HIGHEST);
  assert(indent >= 0);
  assert(pas != NULL);
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lspref_print(fp, prec, indent, pas->lpa_pref);
  lsprintf(fp, indent, "@");
  lspat_print(fp, LSPREC_APPL + 1, indent, pas->lpa_pat);
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

lspres_t lspas_prepare(const lspas_t *pas, lseenv_t *env,
                       const lserref_base_t *erref) {
  assert(pas != NULL);
  assert(env != NULL);
  assert(erref != NULL);
  lspres_t res = lspat_prepare(pas->lpa_pat, env, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  res = lspref_prepare(pas->lpa_pref, env, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  return LSPRES_SUCCESS;
}

const lspref_t *lspas_get_pref(const lspas_t *pas) { return pas->lpa_pref; }

const lspat_t *lspas_get_pat(const lspas_t *pas) { return pas->lpa_pat; }
#include "pat/pas.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "lstypes.h"
#include <assert.h>

struct lspas {
  const lsref_t *lpa_ref;
  const lspat_t *lpa_pat;
};

const lspas_t *lspas_new(const lsref_t *ref, const lspat_t *pat) {
  assert(ref != NULL);
  assert(pat != NULL);
  lspas_t *pas = lsmalloc(sizeof(lspas_t));
  pas->lpa_ref = ref;
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
  lsref_print(fp, prec, indent, pas->lpa_ref);
  lsprintf(fp, indent, "@");
  lspat_print(fp, LSPREC_APPL + 1, indent, pas->lpa_pat);
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

const lsref_t *lspas_get_ref(const lspas_t *pas) { return pas->lpa_ref; }

const lspat_t *lspas_get_pat(const lspas_t *pas) { return pas->lpa_pat; }
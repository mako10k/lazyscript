#include "as.h"
#include "lazyscript.h"

struct lsas {
  const lsstr_t *name;
  const lspat_t *pat;
};

lsas_t *lsas(const lsstr_t *name, const lspat_t *pat) {
  lsas_t *as = malloc(sizeof(lsas_t));
  as->name = name;
  as->pat = pat;
  return as;
}

void lsas_print(FILE *fp, int prec, const lsas_t *as) {
  if (prec > LSPREC_APPL)
    fprintf(fp, "(");
  lsstr_print_bare(fp, as->name);
  fprintf(fp, "@");
  lspat_print(fp, LSPREC_APPL + 1, as->pat);
  if (prec > LSPREC_APPL)
    fprintf(fp, ")");
}
#include "as.h"

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
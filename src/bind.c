#include "bind.h"
#include "lazyscript.h"
#include <assert.h>

struct lsbind {
  lsarray_t *ents;
};

struct lsbind_ent {
  lspat_t *pat;
  lsexpr_t *expr;
};

lsbind_t *lsbind(void) {
  lsbind_t *bind = malloc(sizeof(lsbind_t));
  bind->ents = lsarray();
  return bind;
}
void lsbind_push(lsbind_t *bind, lsbind_ent_t *ent) {
  lsarray_push(bind->ents, ent);
}

lsbind_ent_t *lsbind_ent(lspat_t *pat, lsexpr_t *expr) {
  lsbind_ent_t *ent = malloc(sizeof(lsbind_ent_t));
  ent->pat = pat;
  ent->expr = expr;
  return ent;
}

void lsbind_print(FILE *fp, int prec, int indent, lsbind_t *bind) {
  (void)prec;
  unsigned int size = lsarray_get_size(bind->ents);
  for (unsigned int i = 0; i < size; i++) {
    lsprintf(fp, indent, ";\n");
    lsbind_ent_print(fp, LSPREC_LOWEST, indent, lsarray_get(bind->ents, i));
  }
}

void lsbind_ent_print(FILE *fp, int prec, int indent, lsbind_ent_t *ent) {
  lspat_print(fp, prec, indent, ent->pat);
  lsprintf(fp, indent, " = ");
  lsexpr_print(fp, prec, indent, ent->expr);
}

unsigned int lsbind_get_count(const lsbind_t *bind) {
  assert(bind != NULL);
  return lsarray_get_size(bind->ents);
}
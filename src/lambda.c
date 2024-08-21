#include "lambda.h"
#include "array.h"
#include "lazyscript.h"

struct lslambda {
  lsarray_t *ents;
};

struct lslambda_ent {
  const lspat_t *pat;
  const lsexpr_t *expr;
};

lslambda_t *lslambda(void) {
  lslambda_t *lambda = malloc(sizeof(lslambda_t));
  lambda->ents = lsarray(0);
  return lambda;
}

lslambda_t *lslambda_push(lslambda_t *lambda, lslambda_ent_t *ent) {
  lsarray_push(lambda->ents, ent);
  return lambda;
}

lslambda_ent_t *lslambda_ent(const lspat_t *pat, const lsexpr_t *expr) {
  lslambda_ent_t *ent = malloc(sizeof(lslambda_ent_t));
  ent->pat = pat;
  ent->expr = expr;
  return ent;
}

void lslambda_print(FILE *fp, int prec, int indent, const lslambda_t *lambda) {
  unsigned int size = lsarray_get_size(lambda->ents);
  if (size > 1) {
    prec = LSPREC_LAMBDA + 1;
    lsprintf(fp, indent, "{ ");
  }
  for (size_t i = 0; i < lsarray_get_size(lambda->ents); i++) {
    if (i > 0)
      lsprintf(fp, indent, ";\n  ");
    lslambda_ent_print(fp, prec, indent + 1, lsarray_get(lambda->ents, i));
  }
  if (size > 1) {
    lsprintf(fp, indent, " }");
  }
}

void lslambda_ent_print(FILE *fp, int prec, int indent,
                        const lslambda_ent_t *ent) {
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, ent->pat);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, ent->expr);
}
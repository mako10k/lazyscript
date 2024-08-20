#include "lambda.h"
#include "lazyscript.h"

struct lslambda {
  const lspat_t *pat;
  const lsexpr_t *expr;
};

lslambda_t *lslambda(const lspat_t *pat, const lsexpr_t *expr) {
  lslambda_t *lambda = malloc(sizeof(lslambda_t));
  lambda->pat = pat;
  lambda->expr = expr;
  return lambda;
}

void lslambda_print(FILE *fp, int prec, const lslambda_t *lambda) {
  fprintf(fp, "\\");
  lspat_print(fp, LSPREC_APPL + 1, lambda->pat);
  fprintf(fp, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA, lambda->expr);
}
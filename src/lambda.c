#include "lambda.h"

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
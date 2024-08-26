#include "lambda.h"
#include "array.h"
#include "io.h"
#include "lazyscript.h"
#include <assert.h>

struct lslambda {
  lsarray_t *ents;
};

struct lslambda_ent {
  lspat_t *pat;
  lsexpr_t *expr;
};

lslambda_t *lslambda(void) {
  lslambda_t *lambda = malloc(sizeof(lslambda_t));
  lambda->ents = lsarray();
  return lambda;
}

lslambda_t *lslambda_push(lslambda_t *lambda, lslambda_ent_t *ent) {
  lsarray_push(lambda->ents, ent);
  return lambda;
}

lslambda_ent_t *lslambda_ent(lspat_t *pat, lsexpr_t *expr) {
  lslambda_ent_t *ent = malloc(sizeof(lslambda_ent_t));
  ent->pat = pat;
  ent->expr = expr;
  return ent;
}

void lslambda_print(FILE *fp, int prec, int indent, const lslambda_t *lambda) {
  (void)prec;
  unsigned int size = lsarray_get_size(lambda->ents);
  for (size_t i = 0; i < size; i++) {
    if (i > 0)
      lsprintf(fp, indent, " |\n");
    lslambda_ent_print(fp, LSPREC_LAMBDA + 1, indent,
                       lsarray_get(lambda->ents, i));
  }
}

void lslambda_ent_print(FILE *fp, int prec, int indent,
                        const lslambda_ent_t *ent) {
  (void)prec;
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, ent->pat);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, ent->expr);
}

unsigned int lslambda_get_count(const lslambda_t *lambda) {
  assert(lambda != NULL);
  return lsarray_get_size(lambda->ents);
}

int lslambda_prepare(lslambda_t *lambda, lsenv_t *env) {
  unsigned int size = lsarray_get_size(lambda->ents);
  for (size_t i = 0; i < size; i++) {
    int res = lslambda_ent_prepare(lsarray_get(lambda->ents, i), env);
    if (res < 0)
      return res;
  }
  return 0;
}

int lslambda_ent_prepare(lslambda_ent_t *ent, lsenv_t *env) {
  env = lsenv(env);
  lserref_t *erref = lserref_lambda_ent(ent);
  int res = lspat_prepare(ent->pat, env, erref);
  if (res < 0)
    return res;
  return lsexpr_prepare(ent->expr, env);
}
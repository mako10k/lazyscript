#include "elambda.h"
#include "array.h"
#include "io.h"
#include "lazyscript.h"
#include <assert.h>

struct lselambda {
  lsarray_t *ents;
};

struct lselambda_ent {
  lspat_t *pat;
  lsexpr_t *expr;
};

lselambda_t *lselambda(void) {
  lselambda_t *lambda = malloc(sizeof(lselambda_t));
  lambda->ents = lsarray();
  return lambda;
}

lselambda_t *lselambda_push(lselambda_t *lambda, lselambda_ent_t *ent) {
  lsarray_push(lambda->ents, ent);
  return lambda;
}

lselambda_ent_t *lselambda_ent(lspat_t *pat, lsexpr_t *expr) {
  lselambda_ent_t *ent = malloc(sizeof(lselambda_ent_t));
  ent->pat = pat;
  ent->expr = expr;
  return ent;
}

void lselambda_print(FILE *fp, int prec, int indent, const lselambda_t *lambda) {
  (void)prec;
  unsigned int size = lsarray_get_size(lambda->ents);
  for (size_t i = 0; i < size; i++) {
    if (i > 0)
      lsprintf(fp, indent, " |\n");
    lselambda_ent_print(fp, LSPREC_LAMBDA + 1, indent,
                       lsarray_get(lambda->ents, i));
  }
}

void lselambda_ent_print(FILE *fp, int prec, int indent,
                        const lselambda_ent_t *ent) {
  (void)prec;
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, ent->pat);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, ent->expr);
}

unsigned int lselambda_get_count(const lselambda_t *lambda) {
  assert(lambda != NULL);
  return lsarray_get_size(lambda->ents);
}

int lselambda_prepare(lselambda_t *lambda, lseenv_t *env) {
  unsigned int size = lsarray_get_size(lambda->ents);
  for (size_t i = 0; i < size; i++) {
    int res = lselambda_ent_prepare(lsarray_get(lambda->ents, i), env);
    if (res < 0)
      return res;
  }
  return 0;
}

int lselambda_ent_prepare(lselambda_ent_t *ent, lseenv_t *env) {
  env = lseenv(env);
  lserref_wrapper_t *erref = lserref_wrapper_lambda_ent(ent);
  int res = lspat_prepare(ent->pat, env, erref);
  if (res < 0)
    return res;
  return lsexpr_prepare(ent->expr, env);
}
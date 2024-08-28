#include "expr/elambda.h"
#include "common/array.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lselambda {
  lsarray_t *lel_ents;
};

struct lselambda_entry {
  lspat_t *lele_arg;
  lsexpr_t *lele_body;
};

lselambda_t *lselambda_new(void) {
  lselambda_t *lambda = lsmalloc(sizeof(lselambda_t));
  lambda->lel_ents = lsarray_new();
  return lambda;
}

lselambda_t *lselambda_push(lselambda_t *lambda, lselambda_entry_t *ent) {
  lsarray_push(lambda->lel_ents, ent);
  return lambda;
}

lselambda_entry_t *lselambda_entry_new(lspat_t *arg, lsexpr_t *body) {
  lselambda_entry_t *ent = lsmalloc(sizeof(lselambda_entry_t));
  ent->lele_arg = arg;
  ent->lele_body = body;
  return ent;
}

void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *lambda) {
  (void)prec;
  lssize_t size = lsarray_get_size(lambda->lel_ents);
  for (size_t i = 0; i < size; i++) {
    if (i > 0)
      lsprintf(fp, indent, " |\n");
    lselambda_entry_print(fp, LSPREC_LAMBDA + 1, indent,
                          lsarray_get(lambda->lel_ents, i));
  }
}

void lselambda_entry_print(FILE *fp, lsprec_t prec, int indent,
                           const lselambda_entry_t *ent) {
  (void)prec;
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, ent->lele_arg);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, ent->lele_body);
}

lssize_t lselambda_get_entry_count(const lselambda_t *lambda) {
  assert(lambda != NULL);
  return lsarray_get_size(lambda->lel_ents);
}

lspres_t lselambda_prepare(lselambda_t *lambda, lseenv_t *env) {
  lssize_t size = lsarray_get_size(lambda->lel_ents);
  for (size_t i = 0; i < size; i++) {
    lspres_t res =
        lselambda_entry_prepare(lsarray_get(lambda->lel_ents, i), env);
    if (res != LSPRES_SUCCESS)
      return res;
  }
  return LSPRES_SUCCESS;
}

lspres_t lselambda_entry_prepare(lselambda_entry_t *ent, lseenv_t *env) {
  env = lseenv_new(env);
  lserref_wrapper_t *erref = lserref_wrapper_lambda_ent(ent);
  lspres_t res = lspat_prepare(ent->lele_arg, env, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  return lsexpr_prepare(ent->lele_body, env);
}

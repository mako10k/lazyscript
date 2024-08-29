#include "expr/elambda.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lselambda {
  lspat_t *lele_arg;
  lsexpr_t *lele_body;
};

lselambda_t *lselambda_new(lspat_t *arg, lsexpr_t *body) {
  lselambda_t *ent = lsmalloc(sizeof(lselambda_t));
  ent->lele_arg = arg;
  ent->lele_body = body;
  return ent;
}

void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *ent) {
  (void)prec;
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, ent->lele_arg);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, ent->lele_body);
}

lspres_t lselambda_prepare(lselambda_t *ent, lseenv_t *env) {
  env = lseenv_new(env);
  const lserref_base_t *erref = lserref_base_new_lambda(ent);
  lspres_t res = lspat_prepare(ent->lele_arg, env, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  return lsexpr_prepare(ent->lele_body, env);
}

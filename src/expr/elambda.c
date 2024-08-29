#include "expr/elambda.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lselambda {
  const lspat_t *lel_arg;
  lsexpr_t *lel_body;
};

lselambda_t *lselambda_new(const lspat_t *arg, lsexpr_t *body) {
  lselambda_t *elambda = lsmalloc(sizeof(lselambda_t));
  elambda->lel_arg = arg;
  elambda->lel_body = body;
  return elambda;
}

void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *elambda) {
  (void)prec;
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, elambda->lel_arg);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, elambda->lel_body);
}

lspres_t lselambda_prepare(lselambda_t *elambda, lseenv_t *eenv) {
  eenv = lseenv_new(eenv);
  const lserref_base_t *erref = lserref_base_new_lambda(elambda);
  lspres_t res = lspat_prepare(elambda->lel_arg, eenv, erref);
  if (res != LSPRES_SUCCESS)
    return res;
  return lsexpr_prepare(elambda->lel_body, eenv);
}

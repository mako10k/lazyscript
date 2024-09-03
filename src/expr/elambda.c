#include "expr/elambda.h"
#include "common/io.h"
#include "common/malloc.h"

struct lselambda {
  const lspat_t *lel_param;
  const lsexpr_t *lel_body;
};

const lselambda_t *lselambda_new(const lspat_t *arg, const lsexpr_t *body) {
  lselambda_t *elambda = lsmalloc(sizeof(lselambda_t));
  elambda->lel_param = arg;
  elambda->lel_body = body;
  return elambda;
}

const lspat_t *lselambda_get_param(const lselambda_t *elambda) {
  return elambda->lel_param;
}

const lsexpr_t *lselambda_get_body(const lselambda_t *elambda) {
  return elambda->lel_body;
}

void lselambda_print(FILE *fp, lsprec_t prec, int indent,
                     const lselambda_t *elambda) {
  (void)prec;
  lsprintf(fp, indent, "\\");
  lspat_print(fp, LSPREC_APPL + 1, indent, elambda->lel_param);
  lsprintf(fp, indent, " -> ");
  lsexpr_print(fp, LSPREC_LAMBDA + 1, indent, elambda->lel_body);
}
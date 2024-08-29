#include "expr/elambda.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lselambda {
  const lspat_t *lel_arg;
  const lsexpr_t *lel_body;
};

const lselambda_t *lselambda_new(const lspat_t *arg, const lsexpr_t *body) {
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
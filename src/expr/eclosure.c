#include "expr/eclosure.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"

struct lseclosure {
  const lsexpr_t *lec_expr;
  const lsbind_t *lec_bind;
};

const lseclosure_t *lseclosure_new(const lsexpr_t *expr, const lsbind_t *bind) {
  lseclosure_t *eclosure = lsmalloc(sizeof(lseclosure_t));
  eclosure->lec_expr = expr;
  eclosure->lec_bind = bind;
  return eclosure;
}

void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                      const lseclosure_t *eclosure) {
  lsprintf(stream, indent + 1, "{\n");
  lsexpr_print(stream, prec, indent + 1, eclosure->lec_expr);
  lsbind_print(stream, prec, indent + 1, eclosure->lec_bind);
  lsprintf(stream, indent, "\n}");
}

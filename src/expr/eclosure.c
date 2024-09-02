#include "expr/eclosure.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"

struct lseclosure {
  const lsexpr_t *lec_expr;
  lssize_t lec_bindc;
  const lsbind_t *lec_binds[0];
};

const lseclosure_t *lseclosure_new(const lsexpr_t *expr, size_t bindc,
                                   const lsbind_t *const *binds) {
  lseclosure_t *eclosure =
      lsmalloc(sizeof(lseclosure_t) + bindc * sizeof(lsbind_t *));
  eclosure->lec_expr = expr;
  eclosure->lec_bindc = bindc;
  for (lssize_t i = 0; i < bindc; i++)
    eclosure->lec_binds[i] = binds[i];
  return eclosure;
}

const lsexpr_t *lseclosure_get_expr(const lseclosure_t *eclosure) {
  return eclosure->lec_expr;
}

lssize_t lseclosure_get_bindc(const lseclosure_t *eclosure) {
  return eclosure->lec_bindc;
}

const lsbind_t *const *lseclosure_get_binds(const lseclosure_t *eclosure) {
  return eclosure->lec_binds;
}

void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                      const lseclosure_t *eclosure) {
  lsprintf(stream, indent + 1, "{\n");
  lsexpr_print(stream, prec, indent + 1, eclosure->lec_expr);
  for (lssize_t i = 0; i < eclosure->lec_bindc; i++) {
    lsprintf(stream, indent + 1, ";\n");
    lsbind_print(stream, prec, indent + 1, eclosure->lec_binds[i]);
  }
  lsprintf(stream, indent, "\n}");
}

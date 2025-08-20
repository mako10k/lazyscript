#include "expr/eclosure.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"
#include <assert.h>

struct lseclosure {
  const lsexpr_t* lec_expr;
  lssize_t        lec_bindc;
  const lsbind_t* lec_binds[0];
};

const lseclosure_t* lseclosure_new(const lsexpr_t* expr, size_t bindc,
                                   const lsbind_t* const* binds) {
  assert(bindc == 0 || binds != NULL);
  lseclosure_t* eclosure = lsmalloc(sizeof(lseclosure_t) + bindc * sizeof(lsbind_t*));
#if defined(DEBUG) && defined(DEBUG_VERBOSE)
  lsprintf(stderr, 0, "D: lseclosure_new: closure=%p, expr=%p, bindc=%zu\n", eclosure, expr, bindc);
#endif
  eclosure->lec_expr  = expr;
  eclosure->lec_bindc = bindc;
  for (lssize_t i = 0; i < bindc; i++) {
#if defined(DEBUG) && defined(DEBUG_VERBOSE)
    lsprintf(stderr, 0, "D: lseclosure_new: binds[%zd]=%p\n", i, binds[i]);
#endif
    eclosure->lec_binds[i] = binds[i];
  }
  return eclosure;
}

const lsexpr_t* lseclosure_get_expr(const lseclosure_t* eclosure) { return eclosure->lec_expr; }

lssize_t        lseclosure_get_bindc(const lseclosure_t* eclosure) { return eclosure->lec_bindc; }

const lsbind_t* const* lseclosure_get_binds(const lseclosure_t* eclosure) {
  return eclosure->lec_binds;
}

void lseclosure_print(FILE* stream, lsprec_t prec, int indent, const lseclosure_t* eclosure) {
  lsprintf(stream, indent + 1, "(\n");
  lsexpr_print(stream, prec, indent + 1, eclosure->lec_expr);
  for (lssize_t i = 0; i < eclosure->lec_bindc; i++) {
    lsprintf(stream, indent + 1, ";\n");
    lsbind_print(stream, prec, indent + 1, eclosure->lec_binds[i]);
  }
  lsprintf(stream, indent, "\n)");
}

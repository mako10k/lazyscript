#include "misc/bind.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lsbind {
  const lspat_t*  lbe_lhs;
  const lsexpr_t* lbe_rhs;
};

const lsbind_t* lsbind_new(const lspat_t* lhs, const lsexpr_t* rhs) {
  lsbind_t* bind = lsmalloc(sizeof(lsbind_t));
  bind->lbe_lhs  = lhs;
  bind->lbe_rhs  = rhs;
#if defined(DEBUG) && defined(DEBUG_VERBOSE)
  lsprintf(stderr, 0, "D: lsbind_new: ");
  lsbind_print(stderr, 0, 0, bind);
  lsprintf(stderr, 0, "\n");
#endif
  return bind;
}

void lsbind_print(FILE* fp, lsprec_t prec, int indent, const lsbind_t* bind) {
  lspat_print(fp, prec, indent, bind->lbe_lhs);
  lsprintf(fp, indent, " = ");
  lsexpr_print(fp, prec, indent, bind->lbe_rhs);
}

const lspat_t*  lsbind_get_lhs(const lsbind_t* bind) { return bind->lbe_lhs; }

const lsexpr_t* lsbind_get_rhs(const lsbind_t* bind) { return bind->lbe_rhs; }
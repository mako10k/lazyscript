#include "expr/enslit.h"
#include "common/io.h"
#include "common/malloc.h"

struct lsenslit {
  lssize_t           count;
  const lsstr_t**    names; // length=count
  const lsexpr_t**   exprs; // length=count
};

const lsenslit_t* lsenslit_new(lssize_t n, const lsstr_t* const* names,
                               const lsexpr_t* const* exprs) {
  lsenslit_t* ns = lsmalloc(sizeof(lsenslit_t));
  ns->count      = n < 0 ? 0 : n;
  if (ns->count == 0) { ns->names = NULL; ns->exprs = NULL; return ns; }
  ns->names = lsmalloc(sizeof(lsstr_t*) * (size_t)ns->count);
  ns->exprs = lsmalloc(sizeof(lsexpr_t*) * (size_t)ns->count);
  for (lssize_t i = 0; i < ns->count; i++) { ns->names[i] = names[i]; ns->exprs[i] = exprs[i]; }
  return ns;
}

lssize_t lsenslit_get_count(const lsenslit_t* ns) { return ns->count; }
const lsstr_t* lsenslit_get_name(const lsenslit_t* ns, lssize_t i) { return ns->names[i]; }
const lsexpr_t* lsenslit_get_expr(const lsenslit_t* ns, lssize_t i) { return ns->exprs[i]; }

void lsenslit_print(FILE* fp, lsprec_t prec, int indent, const lsenslit_t* ns) {
  (void)prec;
  lsprintf(fp, indent, "{ ");
  lssize_t n = ns->count;
  for (lssize_t i = 0; i < n; i++) {
    if (i > 0) lsprintf(fp, 0, "; ");
    lsprintf(fp, 0, ".");
    lsstr_print_bare(fp, LSPREC_LOWEST, 0, ns->names[i]);
    lsprintf(fp, 0, " = ");
    lsexpr_print(fp, LSPREC_LOWEST, 0, ns->exprs[i]);
  }
  lsprintf(fp, 0, " }");
}

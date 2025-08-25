#include "expr/enslit.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"

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
  lssize_t n = ns->count;
  if (n == 0) {
    // Empty namespace literal in compact form
    lsprintf(fp, indent, "{ }");
    return;
  }

  // Multi-line pretty print
  lsprintf(fp, indent, "{");
  // Start next line at indent+1
  lsprintf(fp, indent + 1, "\n");
  for (lssize_t i = 0; i < n; i++) {
    // Print name with a single leading '.'; avoid duplicating if already present
    const lsstr_t* name = ns->names[i];
    const char*    nb   = name ? lsstr_get_buf(name) : NULL;
    if (!(nb && nb[0] == '.'))
      lsprintf(fp, 0, ".");
    lsstr_print_bare(fp, LSPREC_LOWEST, 0, name);
    // If RHS is a lambda-chain, print in sugared form: .sym p1 p2 = body
    const lsexpr_t* rhs = ns->exprs[i];
    if (lsexpr_typeof(rhs) == LSEQ_LAMBDA) {
      const lsexpr_t* body = rhs;
      const lspat_t* params[64];
      int pc = 0;
      while (lsexpr_typeof(body) == LSEQ_LAMBDA && pc < 64) {
        const lselambda_t* lam = lsexpr_get_lambda(body);
        params[pc++] = lselambda_get_param(lam);
        body = lselambda_get_body(lam);
      }
      for (int j = 0; j < pc; j++) {
        lsprintf(fp, 0, " ");
        lspat_print(fp, LSPREC_APPL + 1, indent + 1, params[j]);
      }
      lsprintf(fp, 0, " = ");
      lsexpr_print(fp, LSPREC_LOWEST, indent + 1, body);
    } else {
      lsprintf(fp, 0, " = ");
      // Print expression; allow nested printers to indent relative to current level
      lsexpr_print(fp, LSPREC_LOWEST, indent + 1, rhs);
    }
    // End entry and set indentation for next line
    if (i + 1 < n)
      lsprintf(fp, indent + 1, ";\n");
    else
      lsprintf(fp, indent, ";\n");
  }
  lsprintf(fp, indent, "}");
}

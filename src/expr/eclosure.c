#include "expr/eclosure.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"

struct lseclosure {
  lsexpr_t *lec_expr;
  const lsbind_t *lec_bind;
};

lseclosure_t *lseclosure_new(lsexpr_t *expr, const lsbind_t *bind) {
  lseclosure_t *eclosure = lsmalloc(sizeof(lseclosure_t));
  eclosure->lec_expr = expr;
  eclosure->lec_bind = bind;
  return eclosure;
}

void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                      lseclosure_t *eclosure) {
  lsprintf(stream, indent + 1, "{\n");
  lsexpr_print(stream, prec, indent + 1, eclosure->lec_expr);
  lsbind_print(stream, prec, indent + 1, eclosure->lec_bind);
  lsprintf(stream, indent, "\n}");
}

lspres_t lseclosure_prepare(lseclosure_t *eclosure, lseenv_t *eenv) {
  eenv = lseenv_new(eenv);
  for (const lsbelist_t *le = lsbind_get_entries(eclosure->lec_bind);
       le != NULL; le = lsbelist_get_next(le)) {
    const lsbind_entry_t *bind_ent = lsbelist_get(le, 0);
    const lspat_t *lhs = lsbind_entry_get_lhs(bind_ent);
    const lserref_base_t *erref = lserref_base_new_bind_entry(bind_ent);
    lspres_t pres = lspat_prepare(lhs, eenv, erref);
    if (pres != LSPRES_SUCCESS)
      return pres;
  }
  for (const lsbelist_t *le = lsbind_get_entries(eclosure->lec_bind);
       le != NULL; le = lsbelist_get_next(le)) {
    const lsbind_entry_t *bind_ent = lsbelist_get(le, 0);
    lsexpr_t *rhs = lsbind_entry_get_rhs(bind_ent);
    lspres_t pres = lsexpr_prepare(rhs, eenv);
    if (pres != LSPRES_SUCCESS)
      return pres;
  }
  return lsexpr_prepare(eclosure->lec_expr, eenv);
}
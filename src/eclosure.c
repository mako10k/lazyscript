#include "eclosure.h"
#include "expr.h"
#include "io.h"
#include "malloc.h"

struct lseclosure {
  lsexpr_t *lec_expr;
  lsbind_t *lec_bind;
};

lseclosure_t *lseclosure_new(lsexpr_t *expr, lsbind_t *bind) {
  lseclosure_t *eclosure = lsmalloc(sizeof(lseclosure_t));
  eclosure->lec_expr = expr;
  eclosure->lec_bind = bind;
  return eclosure;
}

void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                      lseclosure_t *eclosure) {
  lssize_t lines = lsbind_get_entry_count(eclosure->lec_bind) + 1;
  if (lsexpr_get_type(eclosure->lec_expr) == LSETYPE_LAMBDA)
    lines +=
        lselambda_get_entry_count(lsexpr_get_lambda(eclosure->lec_expr)) - 1;
  if (lines > 1) {
    indent++;
    lsprintf(stream, indent, "{\n");
  } else
    lsprintf(stream, indent, "{ ");
  lsexpr_print(stream, prec, indent, eclosure->lec_expr);
  lsbind_print(stream, prec, indent, eclosure->lec_bind);
  if (lines > 1) {
    indent--;
    lsprintf(stream, indent, "\n}");
  } else
    lsprintf(stream, indent, " }");
}

lspres_t lseclosure_prepare(lseclosure_t *eclosure, lseenv_t *eenv) {
  eenv = lseenv_new(eenv);
  for (const lsbelist_t *le = lsbind_get_entries(eclosure->lec_bind);
       le != NULL; le = lsbelist_get_next(le)) {
    lsbind_entry_t *bind_ent = lsbelist_get(le, 0);
    lspat_t *lhs = lsbind_entry_get_lhs(bind_ent);
    lserref_wrapper_t *erref = lserref_wrapper_bind_ent(bind_ent);
    lspres_t pres = lspat_prepare(lhs, eenv, erref);
    if (pres != LSPRES_SUCCESS)
      return pres;
  }
  for (const lsbelist_t *le = lsbind_get_entries(eclosure->lec_bind);
       le != NULL; le = lsbelist_get_next(le)) {
    lsbind_entry_t *bind_ent = lsbelist_get(le, 0);
    lsexpr_t *rhs = lsbind_entry_get_rhs(bind_ent);
    lspres_t pres = lsexpr_prepare(rhs, eenv);
    if (pres != LSPRES_SUCCESS)
      return pres;
  }
  return lsexpr_prepare(eclosure->lec_expr, eenv);
}

lsthunk_t *lseclosure_thunk(lstenv_t *tenv, const lseclosure_t *eclosure) {
  return lsexpr_thunk(lstenv(tenv), eclosure->lec_expr);
}
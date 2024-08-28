#include "expr/eappl.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"
#include <assert.h>

struct lseappl {
  lsexpr_t *lea_func;
  const lselist_t *lea_args;
};

lseappl_t *lseappl_new(lsexpr_t *func) {
  assert(func != NULL);
  lseappl_t *eappl = lsmalloc(sizeof(lseappl_t));
  eappl->lea_func = func;
  eappl->lea_args = lselist_new();
  return eappl;
}

void lseappl_add_arg(lseappl_t *eappl, lsexpr_t *arg) {
  assert(eappl != NULL);
  eappl->lea_args = lselist_push(eappl->lea_args, arg);
}

void lseappl_concat_args(lseappl_t *eappl, const lselist_t *args) {
  assert(eappl != NULL);
  eappl->lea_args = lselist_concat(eappl->lea_args, args);
}

const lselist_t *lseappl_get_args(const lseappl_t *eappl) {
  assert(eappl != NULL);
  return eappl->lea_args;
}

const lsexpr_t *lseappl_get_func(const lseappl_t *eappl) {
  assert(eappl != NULL);
  return eappl->lea_func;
}

lssize_t lseappl_get_arg_count(const lseappl_t *eappl) {
  assert(eappl != NULL);
  return eappl->lea_args == NULL ? 0 : lselist_count(eappl->lea_args);
}

lsexpr_t *lseappl_get_arg(const lseappl_t *eappl, lssize_t i) {
  assert(eappl != NULL);
  return eappl->lea_args == NULL ? NULL : lselist_get(eappl->lea_args, i);
}

void lseappl_print(FILE *stream, lsprec_t prec, int indent,
                   const lseappl_t *eappl) {
  assert(stream != NULL);
  assert(eappl != NULL);
  lssize_t argc = lselist_count(eappl->lea_args);
  if (argc == 0) {
    lsexpr_print(stream, prec, indent, eappl->lea_func);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, "(");
  lsexpr_print(stream, LSPREC_APPL + 1, indent, eappl->lea_func);
  for (const lselist_t *le = eappl->lea_args; le != NULL;
       le = lselist_get_next(le)) {
    lsprintf(stream, indent, " ");
    lsexpr_print(stream, LSPREC_APPL + 1, indent, lselist_get(le, 0));
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, ")");
}

lspres_t lseappl_prepare(lseappl_t *eappl, lseenv_t *eenv) {
  assert(eappl != NULL);
  assert(eenv != NULL);
  lspres_t pres = lsexpr_prepare(eappl->lea_func, eenv);
  if (pres != LSPRES_SUCCESS)
    return pres;
  lssize_t argc = lselist_count(eappl->lea_args);
  if (argc == 0)
    return LSPRES_SUCCESS;
  for (const lselist_t *le = eappl->lea_args; le != NULL;
       le = lselist_get_next(le)) {
    lsexpr_t *arg = lselist_get(le, 0);
    lspres_t pres = lsexpr_prepare(arg, eenv);
    if (pres != LSPRES_SUCCESS)
      return pres;
  }
  return LSPRES_SUCCESS;
}

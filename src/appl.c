#include "appl.h"
#include "lazyscript.h"
#include "malloc.h"
#include <assert.h>

struct lsappl {
  const lsexpr_t *func;
  lsarray_t *args;
};

lsappl_t *lsappl(const lsexpr_t *func) {
  assert(func != NULL);
  lsappl_t *appl = lsmalloc(sizeof(lsappl_t));
  appl->func = func;
  appl->args = NULL;
  return appl;
}

void lsappl_push_arg(lsappl_t *appl, lsexpr_t *arg) {
  assert(appl != NULL);
  if (appl->args == NULL)
    appl->args = lsarray(0);
  lsarray_push(appl->args, arg);
}

void lsappl_push_args(lsappl_t *appl, lsarray_t *args) {
  assert(appl != NULL);
  if (appl->args == NULL) {
    appl->args = args;
  } else
    appl->args = lsarray_concat(appl->args, args);
}

const lsexpr_t *lsappl_get_func(const lsappl_t *appl) {
  assert(appl != NULL);
  return appl->func;
}

unsigned int lsappl_get_argc(const lsappl_t *appl) {
  assert(appl != NULL);
  return appl->args == NULL ? 0 : lsarray_get_size(appl->args);
}

lsexpr_t *lsappl_get_arg(const lsappl_t *appl, unsigned int i) {
  assert(appl != NULL);
  return appl->args == NULL ? NULL : lsarray_get(appl->args, i);
}

void lsappl_print(FILE *fp, int prec, int indent, const lsappl_t *appl) {
  assert(fp != NULL);
  assert(appl != NULL);
  if (appl->args == NULL || lsarray_get_size(appl->args) == 0) {
    lsexpr_print(fp, prec, indent, appl->func);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lsexpr_print(fp, LSPREC_APPL + 1, indent, appl->func);
  for (unsigned int i = 0; i < lsarray_get_size(appl->args); i++) {
    if (i > 0)
      lsprintf(fp, indent, ", ");
    lsexpr_print(fp, LSPREC_APPL + 1, indent, lsarray_get(appl->args, i));
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}
#include "eappl.h"
#include "expr.h"
#include "io.h"
#include "lazyscript.h"
#include "malloc.h"
#include <assert.h>

struct lseappl {
  lsexpr_t *func;
  lsarray_t *args;
};

lseappl_t *lseappl(lsexpr_t *func) {
  assert(func != NULL);
  lseappl_t *eappl = lsmalloc(sizeof(lseappl_t));
  eappl->func = func;
  eappl->args = NULL;
  return eappl;
}

void lseappl_push_arg(lseappl_t *eappl, lsexpr_t *arg) {
  assert(eappl != NULL);
  if (eappl->args == NULL)
    eappl->args = lsarray();
  lsarray_push(eappl->args, arg);
}

void lseappl_push_args(lseappl_t *eappl, lsarray_t *args) {
  assert(eappl != NULL);
  if (eappl->args == NULL) {
    eappl->args = args;
  } else
    eappl->args = lsarray_concat(eappl->args, args);
}

const lsexpr_t *lseappl_get_func(const lseappl_t *eappl) {
  assert(eappl != NULL);
  return eappl->func;
}

unsigned int lseappl_get_argc(const lseappl_t *eappl) {
  assert(eappl != NULL);
  return eappl->args == NULL ? 0 : lsarray_get_size(eappl->args);
}

lsexpr_t *lseappl_get_arg(const lseappl_t *eappl, unsigned int i) {
  assert(eappl != NULL);
  return eappl->args == NULL ? NULL : lsarray_get(eappl->args, i);
}

void lseappl_print(FILE *stream, int prec, int indent, const lseappl_t *eappl) {
  assert(stream != NULL);
  assert(eappl != NULL);
  if (eappl->args == NULL || lsarray_get_size(eappl->args) == 0) {
    lsexpr_print(stream, prec, indent, eappl->func);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, "(");
  lsexpr_print(stream, LSPREC_APPL + 1, indent, eappl->func);
  for (unsigned int i = 0; i < lsarray_get_size(eappl->args); i++) {
    lsprintf(stream, indent, " ");
    lsexpr_print(stream, LSPREC_APPL + 1, indent, lsarray_get(eappl->args, i));
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, ")");
}

int lseappl_prepare(lseappl_t *eappl, lseenv_t *eenv) {
  assert(eappl != NULL);
  assert(eenv != NULL);
  if (eappl->args == NULL)
    return 0;
  for (unsigned int i = 0; i < lsarray_get_size(eappl->args); i++) {
    lsexpr_t *arg = lsarray_get(eappl->args, i);
    int res = lsexpr_prepare(arg, eenv);
    if (res < 0)
      return res;
  }
  return 0;
}

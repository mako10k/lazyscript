#include "expr/eappl.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"

struct lseappl {
  const lsexpr_t *lea_func;
  lssize_t lea_argc;
  const lsexpr_t *lea_args[0];
};

const lseappl_t *lseappl_new(const lsexpr_t *func, size_t argc,
                             const lsexpr_t *args[]) {
  lseappl_t *eappl = lsmalloc(sizeof(lseappl_t));
  eappl->lea_func = func;
  eappl->lea_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    eappl->lea_args[i] = args[i];
  return eappl;
}

const lseappl_t *lseappl_add_arg(const lseappl_t *eappl, const lsexpr_t *arg) {
  lssize_t argc = eappl->lea_argc;
  lseappl_t *eappl_new =
      lsmalloc(sizeof(lseappl_t) + (argc + 1) * sizeof(lsexpr_t *));
  eappl_new->lea_func = eappl->lea_func;
  eappl_new->lea_argc = argc + 1;
  for (lssize_t i = 0; i < argc; i++)
    eappl_new->lea_args[i] = eappl->lea_args[i];
  eappl_new->lea_args[argc] = arg;
  return eappl_new;
}

const lseappl_t *lseappl_concat_args(lseappl_t *eappl, size_t argc,
                                     const lsexpr_t *args[]) {
  lssize_t argc0 = eappl->lea_argc;
  lseappl_t *eappl_new =
      lsmalloc(sizeof(lseappl_t) + (argc0 + argc) * sizeof(lsexpr_t *));
  eappl_new->lea_func = eappl->lea_func;
  eappl_new->lea_argc = argc0 + argc;
  for (lssize_t i = 0; i < argc0; i++)
    eappl_new->lea_args[i] = eappl->lea_args[i];
  for (lssize_t i = 0; i < argc; i++)
    eappl_new->lea_args[argc0 + i] = args[i];
  return eappl_new;
}

const lsexpr_t *lseappl_get_func(const lseappl_t *eappl) {
  return eappl->lea_func;
}

lssize_t lseappl_get_argc(const lseappl_t *eappl) { return eappl->lea_argc; }

const lsexpr_t *const *lseappl_get_args(const lseappl_t *eappl) {
  return eappl->lea_args;
}

void lseappl_print(FILE *stream, lsprec_t prec, int indent,
                   const lseappl_t *eappl) {
  lssize_t argc = eappl->lea_argc;
  if (argc == 0) {
    lsexpr_print(stream, prec, indent, eappl->lea_func);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, "(");
  lsexpr_print(stream, LSPREC_APPL + 1, indent, eappl->lea_func);
  for (size_t i = 0; i < argc; i++) {
    lsprintf(stream, indent, " ");
    lsexpr_print(stream, LSPREC_APPL + 1, indent, eappl->lea_args[i]);
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, ")");
}
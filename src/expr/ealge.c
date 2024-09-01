#include "expr/ealge.h"
#include "common/io.h"
#include "common/malloc.h"

struct lsealge {
  const lsstr_t *lea_constr;
  lssize_t lea_argc;
  const lsexpr_t *lea_args[0];
};

const lsealge_t *lsealge_new(const lsstr_t *constr, lssize_t argc,
                             const lsexpr_t *const *args) {
  lsealge_t *ealge = lsmalloc(sizeof(lsealge_t) + argc * sizeof(lsexpr_t *));
  ealge->lea_constr = constr;
  ealge->lea_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    ealge->lea_args[i] = args[i];
  return ealge;
}

const lsealge_t *lsealge_add_arg(const lsealge_t *ealge, const lsexpr_t *arg) {
  lssize_t argc = ealge->lea_argc;
  lsealge_t *ealge_new =
      lsmalloc(sizeof(lsealge_t) + (argc + 1) * sizeof(lsexpr_t *));
  ealge_new->lea_constr = ealge->lea_constr;
  ealge_new->lea_argc = argc + 1;
  for (lssize_t i = 0; i < argc; i++)
    ealge_new->lea_args[i] = ealge->lea_args[i];
  ealge_new->lea_args[argc] = arg;
  return ealge_new;
}

const lsealge_t *lsealge_concat_args(const lsealge_t *ealge, lssize_t argc,
                                     const lsexpr_t *args[]) {
  lssize_t argc0 = ealge->lea_argc;
  lsealge_t *ealge_new =
      lsmalloc(sizeof(lsealge_t) + (argc0 + argc) * sizeof(lsexpr_t *));
  ealge_new->lea_constr = ealge->lea_constr;
  ealge_new->lea_argc = argc0 + argc;
  for (lssize_t i = 0; i < argc0; i++)
    ealge_new->lea_args[i] = ealge->lea_args[i];
  for (lssize_t i = 0; i < argc; i++)
    ealge_new->lea_args[argc0 + i] = args[i];
  return ealge_new;
}

const lsstr_t *lsealge_get_constr(const lsealge_t *ealge) {
  return ealge->lea_constr;
}

lssize_t lsealge_get_argc(const lsealge_t *ealge) { return ealge->lea_argc; }

const lsexpr_t *lsealge_get_arg(const lsealge_t *ealge, int i) {
  return ealge->lea_args[i];
}

const lsexpr_t *const *lsealge_get_args(const lsealge_t *ealge) {
  return ealge->lea_args;
}

void lsealge_print(FILE *fp, lsprec_t prec, int indent,
                   const lsealge_t *ealge) {
  static const lsstr_t *cons_constr = NULL;
  static const lsstr_t *list_constr = NULL;
  static const lsstr_t *tuple_constr = NULL;
  if (cons_constr == NULL) {
    cons_constr = lsstr_cstr(":");
    list_constr = lsstr_cstr("[]");
    tuple_constr = lsstr_cstr(",");
  }
  lssize_t argc = ealge->lea_argc;
  if (lsstrcmp(ealge->lea_constr, cons_constr) == 0 && argc == 2) {
    if (prec > LSPREC_CONS)
      lsprintf(fp, indent, "(");
    lsexpr_print(fp, LSPREC_CONS + 1, indent, ealge->lea_args[0]);
    lsprintf(fp, indent, " : ");
    lsexpr_print(fp, LSPREC_CONS, indent, ealge->lea_args[1]);
    if (prec > LSPREC_CONS)
      lsprintf(fp, indent, ")");
    return;
  }
  if (lsstrcmp(ealge->lea_constr, list_constr) == 0) {
    lsprintf(fp, indent, "[");
    for (lssize_t i = 0; i < argc; i++) {
      if (i > 0)
        lsprintf(fp, indent, ", ");
      lsexpr_print(fp, LSPREC_LOWEST, indent, ealge->lea_args[i]);
    }
    lsprintf(fp, indent, "]");
    return;
  }
  if (lsstrcmp(ealge->lea_constr, tuple_constr) == 0) {
    lsprintf(fp, indent, "(");
    for (lssize_t i = 0; i < argc; i++) {
      if (i > 0)
        lsprintf(fp, indent, ", ");
      lsexpr_print(fp, LSPREC_LOWEST, indent, ealge->lea_args[i]);
    }
    lsprintf(fp, indent, ")");
    return;
  }

  if (argc == 0) {
    lsstr_print_bare(fp, prec, indent, ealge->lea_constr);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lsstr_print_bare(fp, LSPREC_APPL + 1, indent, ealge->lea_constr);
  for (lssize_t i = 0; i < argc; i++) {
    fprintf(fp, " ");
    lsexpr_print(fp, LSPREC_APPL + 1, indent, ealge->lea_args[i]);
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}
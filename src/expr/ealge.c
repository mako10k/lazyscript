#include "ealge.h"
#include "elist.h"
#include "io.h"
#include "malloc.h"
#include <assert.h>

struct lsealge {
  const lsstr_t *leal_constr;
  const lselist_t *leal_args;
};

lsealge_t *lsealge_new(const lsstr_t *constr) {
  assert(constr != NULL);
  lsealge_t *ealge = lsmalloc(sizeof(lsealge_t));
  ealge->leal_constr = constr;
  ealge->leal_args = lselist_new();
  return ealge;
}

void lsealge_add_arg(lsealge_t *ealge, lsexpr_t *arg) {
  assert(ealge != NULL);
  ealge->leal_args = lselist_push(ealge->leal_args, arg);
}

void lsealge_concat_args(lsealge_t *ealge, const lselist_t *args) {
  assert(ealge != NULL);
  ealge->leal_args = lselist_concat(ealge->leal_args, args);
}

const lsstr_t *lsealge_get_constr(const lsealge_t *ealge) {
  assert(ealge != NULL);
  return ealge->leal_constr;
}

lssize_t lsealge_get_arg_count(const lsealge_t *ealge) {
  assert(ealge != NULL);
  return lselist_count(ealge->leal_args);
}

lsexpr_t *lsealge_get_arg(const lsealge_t *ealge, int i) {
  assert(ealge != NULL);
  return lselist_get(ealge->leal_args, i);
}

void lsealge_print(FILE *fp, lsprec_t prec, int indent,
                   const lsealge_t *ealge) {
  assert(ealge != NULL);
  lssize_t argc = lselist_count(ealge->leal_args);
  if (argc == 0) {
    lsstr_print_bare(fp, prec, indent, ealge->leal_constr);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lsstr_print_bare(fp, LSPREC_APPL + 1, indent, ealge->leal_constr);
  for (lssize_t i = 0; i < argc; i++) {
    if (i > 0)
      fprintf(fp, " ");
    lsexpr_print(fp, LSPREC_APPL + 1, indent, lselist_get(ealge->leal_args, i));
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

lspres_t lsealge_prepare(lsealge_t *ealge, lseenv_t *env) {
  assert(ealge != NULL);
  lssize_t argc = lselist_count(ealge->leal_args);
  for (lssize_t i = 0; i < argc; i++) {
    lsexpr_t *arg = lselist_get(ealge->leal_args, i);
    lspres_t pres = lsexpr_prepare(arg, env);
    if (pres != LSPRES_SUCCESS)
      return pres;
  }
  return LSPRES_SUCCESS;
}

const lselist_t *lsealge_get_args(const lsealge_t *ealge) {
  assert(ealge != NULL);
  return ealge->leal_args;
}
#include "pat/palge.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "lstypes.h"
#include "pat/pat.h"
#include <assert.h>

struct lspalge {
  const lsstr_t *lpa_constr;
  lssize_t lpa_argc;
  const lspat_t *lpa_args[0];
};

const lspalge_t *lspalge_new(const lsstr_t *constr, lssize_t argc,
                             const lspat_t *const *args) {
  assert(argc == 0 || args != NULL);
  lspalge_t *palge =
      lsmalloc(sizeof(lspalge_t) + argc * sizeof(const lspat_t *));
  palge->lpa_constr = constr;
  palge->lpa_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    palge->lpa_args[i] = args[i];
  return palge;
}

const lspalge_t *lspalge_add_arg(const lspalge_t *palge, const lspat_t *arg) {
  lssize_t argc = palge->lpa_argc;
  lspalge_t *new_palge =
      lsmalloc(sizeof(lspalge_t) + (argc + 1) * sizeof(const lspat_t *));
  new_palge->lpa_constr = palge->lpa_constr;
  new_palge->lpa_argc = argc + 1;
  for (lssize_t i = 0; i < argc; i++)
    new_palge->lpa_args[i] = palge->lpa_args[i];
  new_palge->lpa_args[argc] = arg;
  return new_palge;
}

const lspalge_t *lspalge_concat_args(const lspalge_t *palge, lssize_t argc,
                                     const lspat_t *const *args) {
  assert(argc == 0 || args != NULL);
  lssize_t argc0 = palge->lpa_argc;
  lspalge_t *palge0 =
      lsmalloc(sizeof(lspalge_t) + (argc0 + argc) * sizeof(const lspat_t *));
  palge0->lpa_constr = palge->lpa_constr;
  palge0->lpa_argc = argc0 + argc;
  for (lssize_t i = 0; i < argc0; i++)
    palge0->lpa_args[i] = palge->lpa_args[i];
  for (lssize_t i = 0; i < argc; i++)
    palge0->lpa_args[argc0 + i] = args[i];
  return palge0;
}

const lsstr_t *lspalge_get_constr(const lspalge_t *palge) {
  return palge->lpa_constr;
}

lssize_t lspalge_get_arg_count(const lspalge_t *palge) {
  return palge->lpa_argc;
}

const lspat_t *lspalge_get_arg(const lspalge_t *alge, int i) {
  return alge->lpa_args[i];
}

void lspalge_print(FILE *fp, int prec, int indent, const lspalge_t *palge) {
  assert(palge != NULL);
  static const lsstr_t *cons_constr = NULL;
  static const lsstr_t *list_constr = NULL;
  static const lsstr_t *tuple_constr = NULL;
  if (cons_constr == NULL) {
    cons_constr = lsstr_cstr(":");
    list_constr = lsstr_cstr("[]");
    tuple_constr = lsstr_cstr(",");
  }
  lssize_t argc = palge->lpa_argc;
  if (lsstrcmp(palge->lpa_constr, cons_constr) == 0 && argc == 2) {
    if (prec > LSPREC_CONS)
      lsprintf(fp, indent, "(");
    lspat_print(fp, LSPREC_CONS + 1, indent, palge->lpa_args[0]);
    lsprintf(fp, indent, " : ");
    lspat_print(fp, LSPREC_CONS, indent, palge->lpa_args[1]);
    if (prec > LSPREC_CONS)
      lsprintf(fp, indent, ")");
    return;
  }
  if (lsstrcmp(palge->lpa_constr, list_constr) == 0) {
    lsprintf(fp, indent, "[");
    for (lssize_t i = 0; i < argc; i++) {
      if (i > 0)
        lsprintf(fp, indent, ", ");
      lspat_print(fp, LSPREC_LOWEST, indent, palge->lpa_args[i]);
    }
    lsprintf(fp, indent, "]");
    return;
  }
  if (lsstrcmp(palge->lpa_constr, tuple_constr) == 0) {
    lsprintf(fp, indent, "(");
    for (lssize_t i = 0; i < argc; i++) {
      if (i > 0)
        lsprintf(fp, indent, ", ");
      lspat_print(fp, LSPREC_LOWEST, indent, palge->lpa_args[i]);
    }
    lsprintf(fp, indent, ")");
    return;
  }

  if (argc == 0) {
    lsstr_print_bare(fp, prec, indent, palge->lpa_constr);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lsstr_print_bare(fp, LSPREC_APPL + 1, indent, palge->lpa_constr);
  for (lssize_t i = 0; i < argc; i++) {
    fprintf(fp, " ");
    lspat_print(fp, LSPREC_APPL + 1, indent, palge->lpa_args[i]);
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

const lspat_t *const *lspalge_get_args(const lspalge_t *alge) {
  assert(alge != NULL);
  return alge->lpa_args;
}
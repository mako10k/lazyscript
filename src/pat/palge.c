#include "pat/palge.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "pat/plist.h"
#include <assert.h>

struct lspalge {
  const lsstr_t *lpa_constr;
  const lsplist_t *lpa_args;
};

lspalge_t *lspalge_new(const lsstr_t *constr) {
  assert(constr != NULL);
  lspalge_t *palge = lsmalloc(sizeof(lspalge_t));
  palge->lpa_constr = constr;
  palge->lpa_args = lsplist_new();
  return palge;
}

void lsexpr_add_args(lspalge_t *palge, const lspat_t *arg) {
  assert(palge != NULL);
  assert(arg != NULL);
  palge->lpa_args = lsplist_push(palge->lpa_args, arg);
}

void lsexpr_concat_args(lspalge_t *palge, const lsplist_t *args) {
  assert(palge != NULL);
  palge->lpa_args = lsplist_concat(palge->lpa_args, args);
}

const lsstr_t *lspalge_get_constr(const lspalge_t *palge) {
  assert(palge != NULL);
  return palge->lpa_constr;
}

lssize_t lspalge_get_arg_count(const lspalge_t *palge) {
  assert(palge != NULL);
  return lsplist_count(palge->lpa_args);
}

const lspat_t *lspalge_get_arg(const lspalge_t *alge, int i) {
  assert(alge != NULL);
  return lsplist_get(alge->lpa_args, i);
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
  lssize_t argc = lsplist_count(palge->lpa_args);
  if (lsstrcmp(palge->lpa_constr, cons_constr) == 0 && argc == 2) {
    if (prec > LSPREC_CONS)
      lsprintf(fp, indent, "(");
    lspat_print(fp, LSPREC_CONS + 1, indent, lsplist_get(palge->lpa_args, 0));
    lsprintf(fp, indent, " : ");
    lspat_print(fp, LSPREC_CONS, indent, lsplist_get(palge->lpa_args, 1));
    if (prec > LSPREC_CONS)
      lsprintf(fp, indent, ")");
    return;
  }
  if (lsstrcmp(palge->lpa_constr, list_constr) == 0) {
    lsprintf(fp, indent, "[");
    for (lssize_t i = 0; i < argc; i++) {
      if (i > 0)
        lsprintf(fp, indent, ", ");
      lspat_print(fp, LSPREC_LOWEST, indent, lsplist_get(palge->lpa_args, i));
    }
    lsprintf(fp, indent, "]");
    return;
  }
  if (lsstrcmp(palge->lpa_constr, tuple_constr) == 0) {
    lsprintf(fp, indent, "(");
    for (lssize_t i = 0; i < argc; i++) {
      if (i > 0)
        lsprintf(fp, indent, ", ");
      lspat_print(fp, LSPREC_LOWEST, indent, lsplist_get(palge->lpa_args, i));
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
    lspat_print(fp, LSPREC_APPL + 1, indent, lsplist_get(palge->lpa_args, i));
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

const lsplist_t *lspalge_get_args(const lspalge_t *alge) {
  assert(alge != NULL);
  return alge->lpa_args;
}
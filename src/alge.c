#include "alge.h"
#include "array.h"
#include "lazyscript.h"
#include "malloc.h"
#include <assert.h>

struct lsalge {
  const lsstr_t *constr;
  lsarray_t *args;
};

lsalge_t *lsalge(const lsstr_t *constr) {
  assert(constr != NULL);
  lsalge_t *alge = lsmalloc(sizeof(lsalge_t));
  alge->constr = constr;
  alge->args = NULL;
  return alge;
}

void lsalge_push_arg(lsalge_t *alge, void *arg) {
  assert(alge != NULL);
  assert(arg != NULL);
  if (alge->args == NULL)
    alge->args = lsarray(0);
  lsarray_push(alge->args, arg);
}

void lsalge_push_args(lsalge_t *alge, const lsarray_t *args) {
  assert(alge != NULL);
  assert(args != NULL);
  if (alge->args == NULL)
    alge->args = lsarray_clone(args);
  else
    alge->args = lsarray_concat(alge->args, args);
}

const lsstr_t *lsalge_get_constr(const lsalge_t *alge) {
  assert(alge != NULL);
  return alge->constr;
}

unsigned int lsalge_get_argc(const lsalge_t *alge) {
  assert(alge != NULL);
  return alge->args == NULL ? 0 : lsarray_get_size(alge->args);
}

void *lsalge_get_arg(const lsalge_t *alge, unsigned int i) {
  assert(alge != NULL);
  return alge->args == NULL ? NULL : lsarray_get(alge->args, i);
}

void lsalge_print(FILE *fp, int prec, const lsalge_t *alge,
                  void (*lsprint)(FILE *, int, const void *)) {
  assert(fp != NULL);
  assert(alge != NULL);
  assert(lsprint != NULL);
  const lsstr_t *constr = lsalge_get_constr(alge);
  if (lsstrcmp(constr, lsstr_cstr(",")) == 0) {
    fprintf(fp, "(");
    for (unsigned int i = 0; i < lsarray_get_size(alge->args); i++) {
      if (i > 0)
        fprintf(fp, ", ");
      lsprint(fp, LSPREC_LOWEST, lsarray_get(alge->args, i));
    }
    fprintf(fp, ")");
    return;
  }
  if (lsstrcmp(constr, lsstr_cstr("[]")) == 0) {
    fprintf(fp, "[");
    for (unsigned int i = 0; i < lsarray_get_size(alge->args); i++) {
      if (i > 0)
        fprintf(fp, ", ");
      lsprint(fp, LSPREC_LOWEST, lsarray_get(alge->args, i));
    }
    fprintf(fp, "]");
    return;
  }
  if (lsstrcmp(constr, lsstr_cstr(":")) == 0 &&
      lsarray_get_size(alge->args) == 2) {
    if (prec > LSPREC_CONS)
      fprintf(fp, "(");
    lsprint(fp, LSPREC_CONS + 1, lsarray_get(alge->args, 0));
    fprintf(fp, " : ");
    lsprint(fp, LSPREC_CONS, lsarray_get(alge->args, 1));
    if (prec > LSPREC_CONS)
      fprintf(fp, ")");
    return;
  }

  if (alge->args == NULL || lsarray_get_size(alge->args) == 0) {
    lsstr_print_bare(fp, alge->constr);
    return;
  }
  if (prec > LSPREC_APPL)
    fprintf(fp, "(");
  lsstr_print_bare(fp, alge->constr);
  for (unsigned int i = 0; i < lsarray_get_size(alge->args); i++) {
    fprintf(fp, " ");
    lsprint(fp, LSPREC_APPL + 1, lsarray_get(alge->args, i));
  }
  if (prec > LSPREC_APPL)
    fprintf(fp, ")");
}
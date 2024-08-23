#include "ealge.h"
#include "alge.h"
#include "malloc.h"
#include <assert.h>

struct lsealge {
  lsalge_t *alge;
};

lsealge_t *lsealge(const lsstr_t *constr) {
  assert(constr != NULL);
  lsealge_t *alge = lsmalloc(sizeof(lsealge_t));
  alge->alge = lsalge(constr);
  return alge;
}

void lsealge_push_arg(lsealge_t *alge, lsexpr_t *arg) {
  lsalge_push_arg(alge->alge, arg);
}

void lsealge_push_args(lsealge_t *alge, const lsarray_t *args) {
  lsalge_push_args(alge->alge, args);
}

const lsstr_t *lsealge_get_constr(const lsealge_t *alge) {
  return lsalge_get_constr(alge->alge);
}

unsigned int lsealge_get_argc(const lsealge_t *alge) {
  return lsalge_get_argc(alge->alge);
}

lsexpr_t *lsealge_get_arg(const lsealge_t *alge, int i) {
  return lsalge_get_arg(alge->alge, i);
}

static void lsexpr_print_callback(FILE *fp, int prec, int indent,
                                  const void *expr) {
  lsexpr_print(fp, prec, indent, expr);
}

void lsealge_print(FILE *fp, int prec, int indent, const lsealge_t *alge) {
  lsalge_print(fp, prec, indent, alge->alge, lsexpr_print_callback);
}

static int lsexpr_prepare_callback(void *expr, lsenv_t *env, lserref_t *erref) {
  (void)erref;
  return lsexpr_prepare(expr, env);
}

int lsealge_prepare(lsealge_t *alge, lsenv_t *env) {
  return lsalge_prepare(alge->alge, env, NULL, lsexpr_prepare_callback);
}
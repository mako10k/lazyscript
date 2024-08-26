#include "ealge.h"
#include "alge.h"
#include "malloc.h"
#include <assert.h>

struct lsealge {
  lsalge_t *alge;
};

lsealge_t *lsealge(const lsstr_t *constr) {
  assert(constr != NULL);
  lsealge_t *ealge = lsmalloc(sizeof(lsealge_t));
  ealge->alge = lsalge(constr);
  return ealge;
}

void lsealge_push_arg(lsealge_t *ealge, lsexpr_t *arg) {
  lsalge_push_arg(ealge->alge, arg);
}

void lsealge_push_args(lsealge_t *ealge, const lsarray_t *args) {
  lsalge_push_args(ealge->alge, args);
}

const lsstr_t *lsealge_get_constr(const lsealge_t *ealge) {
  return lsalge_get_constr(ealge->alge);
}

unsigned int lsealge_get_argc(const lsealge_t *ealge) {
  return lsalge_get_argc(ealge->alge);
}

lsexpr_t *lsealge_get_arg(const lsealge_t *ealge, int i) {
  return lsalge_get_arg(ealge->alge, i);
}

static void lsexpr_print_callback(FILE *fp, int prec, int indent,
                                  const void *expr) {
  lsexpr_print(fp, prec, indent, expr);
}

void lsealge_print(FILE *fp, int prec, int indent, const lsealge_t *ealge) {
  lsalge_print(fp, prec, indent, ealge->alge, lsexpr_print_callback);
}

static int lsexpr_prepare_callback(void *ealge, lseenv_t *env,
                                   lserref_wrapper_t *erref) {
  (void)erref;
  return lsexpr_prepare(ealge, env);
}

int lsealge_prepare(lsealge_t *ealge, lseenv_t *env) {
  return lsalge_prepare(ealge->alge, env, NULL, lsexpr_prepare_callback);
}
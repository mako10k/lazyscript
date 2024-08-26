
#include "palge.h"
#include "alge.h"
#include "array.h"
#include <assert.h>

struct lspalge {
  lsalge_t *alge;
};

lspalge_t *lspalge(const lsstr_t *constr) {
  assert(constr != NULL);
  lspalge_t *palge = malloc(sizeof(lspalge_t));
  palge->alge = lsalge(constr);
  return palge;
}

void lspalge_push_arg(lspalge_t *palge, lspat_t *arg) {
  assert(palge != NULL);
  assert(arg != NULL);
  lsalge_push_arg(palge->alge, arg);
}

void lspalge_push_args(lspalge_t *palge, const lsarray_t *args) {
  assert(palge != NULL);
  lsalge_push_args(palge->alge, args);
}

const lsstr_t *lspalge_get_constr(const lspalge_t *palge) {
  assert(palge != NULL);
  return lsalge_get_constr(palge->alge);
}

unsigned int lspalge_get_argc(const lspalge_t *palge) {
  assert(palge != NULL);
  return lsalge_get_argc(palge->alge);
}

lspat_t *lspalge_get_arg(const lspalge_t *alge, int i) {
  assert(alge != NULL);
  return lsalge_get_arg(alge->alge, i);
}

void lspalge_print(FILE *fp, int prec, int indent, const lspalge_t *alge) {
  assert(alge != NULL);
  lsalge_print(fp, prec, indent, alge->alge, (lsalge_print_t)lspat_print);
}

int lspalge_prepare(lspalge_t *alge, lseenv_t *env, lserref_wrapper_t *erref) {
  assert(alge != NULL);
  return lsalge_prepare(alge->alge, env, erref,
                        (lsalge_prepare_t)lspat_prepare);
}
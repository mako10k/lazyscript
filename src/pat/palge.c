#include "pat/palge.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "pat/plist.h"
#include "thunk/thunk.h"
#include <assert.h>

struct lspalge {
  const lsstr_t *lpal_constr;
  const lsplist_t *lpal_args;
};

lspalge_t *lspalge_new(const lsstr_t *constr) {
  assert(constr != NULL);
  lspalge_t *palge = lsmalloc(sizeof(lspalge_t));
  palge->lpal_constr = constr;
  palge->lpal_args = lsplist_new();
  return palge;
}

void lsexpr_add_args(lspalge_t *palge, lspat_t *arg) {
  assert(palge != NULL);
  assert(arg != NULL);
  palge->lpal_args = lsplist_push(palge->lpal_args, arg);
}

void lsexpr_concat_args(lspalge_t *palge, const lsplist_t *args) {
  assert(palge != NULL);
  palge->lpal_args = lsplist_concat(palge->lpal_args, args);
}

const lsstr_t *lspalge_get_constr(const lspalge_t *palge) {
  assert(palge != NULL);
  return palge->lpal_constr;
}

lssize_t lspalge_get_arg_count(const lspalge_t *palge) {
  assert(palge != NULL);
  return lsplist_count(palge->lpal_args);
}

lspat_t *lspalge_get_arg(const lspalge_t *alge, int i) {
  assert(alge != NULL);
  return lsplist_get(alge->lpal_args, i);
}

void lspalge_print(FILE *fp, int prec, int indent, const lspalge_t *alge) {
  assert(alge != NULL);
  lssize_t argc = lsplist_count(alge->lpal_args);
  if (argc == 0) {
    lsstr_print_bare(fp, prec, indent, alge->lpal_constr);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, "(");
  lsstr_print_bare(fp, LSPREC_APPL + 1, indent, alge->lpal_constr);
  for (lssize_t i = 0; i < argc; i++) {
    if (i > 0)
      fprintf(fp, " ");
    lspat_print(fp, LSPREC_APPL + 1, indent, lsplist_get(alge->lpal_args, i));
  }
  if (prec > LSPREC_APPL)
    lsprintf(fp, indent, ")");
}

lspres_t lspalge_prepare(lspalge_t *alge, lseenv_t *env,
                         lserref_wrapper_t *erref) {
  assert(alge != NULL);
  lssize_t argc = lsplist_count(alge->lpal_args);
  for (lssize_t i = 0; i < argc; i++) {
    lspat_t *arg = lsplist_get(alge->lpal_args, i);
    lspres_t res = lspat_prepare(arg, env, erref);
    if (res != LSPRES_SUCCESS)
      return res;
  }
  return LSPRES_SUCCESS;
}

lsmres_t lspalge_match(lstenv_t *tenv, const lspalge_t *palge,
                       lsthunk_t *thunk) {
  assert(palge != NULL);
  lsthunk_t *thunk_whnf = lsthunk_get_whnf(thunk);
  lsttype_t ttype = lsthunk_type(thunk_whnf);
  if (ttype != LSTTYPE_ALGE)
    return LSMATCH_FAILURE;
  lstalge_t *talge = lsthunk_get_alge(thunk_whnf);
  const lsstr_t *tconstr = lstalge_get_constr(talge);
  if (lsstrcmp(palge->lpal_constr, tconstr) != 0)
    return LSMATCH_FAILURE;
  lssize_t pargc = lsplist_count(palge->lpal_args);
  lssize_t targc = lstalge_get_argc(talge);
  if (pargc != targc)
    return LSMATCH_FAILURE;
  for (lssize_t i = 0; i < pargc; i++) {
    lspat_t *parg = lsplist_get(palge->lpal_args, i);
    lsthunk_t *targ = lstalge_get_arg(talge, i);
    if (lspat_match(tenv, parg, targ) < 0)
      return LSMATCH_FAILURE;
  }
  return LSMATCH_SUCCESS;
}

const lsplist_t *lspalge_get_args(const lspalge_t *alge) {
  assert(alge != NULL);
  return alge->lpal_args;
}
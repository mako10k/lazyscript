#include "thunk/tref.h"
#include "common/malloc.h"
#include "expr/eref.h"
#include "misc/bind.h"
#include "thunk/tenv.h"

struct lstref {
  /** origin information */
  const lseref_t *ltr_eref;
  lstenv_t *ltr_env;
  /** thunk reference */
  lsthunk_t *ltr_bound;
};

lstref_t *lstref_new(const lseref_t *eref) {
  lstref_t *tref = lsmalloc(sizeof(lstref_t));
  tref->ltr_eref = eref;
  tref->ltr_env = NULL;
  tref->ltr_bound = NULL;
  return tref;
}

lsthunk_t *lstref_eval(lstref_t *tref) {
  if (tref->ltr_bound != NULL)
    return lsthunk_eval(tref->ltr_bound);
  lserref_t *erref = lseref_get_erref(tref->ltr_eref);
  switch (lserref_get_type(erref)) {
  case LSERRTYPE_BIND_ENTRY: {
    if (tref->ltr_env == NULL) {
      tref->ltr_env = lstenv_new(NULL);
      const lsbind_entry_t *bentry = lserref_get_bind_entry(erref);
      const lspat_t *lhs = lsbind_entry_get_lhs(bentry);
      const lsexpr_t *erhs = lsbind_entry_get_rhs(bentry);
      lsthunk_t *trhs = lsthunk_new_expr(erhs);
      lsmres_t mres = lsthunk_match_pat(trhs, lhs, tref->ltr_env);
      if (mres != LSMATCH_SUCCESS)
        return NULL;
    }
    tref->ltr_bound =
        lstenv_get(tref->ltr_env, lseref_get_name(tref->ltr_eref));
    return lsthunk_eval(tref->ltr_bound);
  }
  case LSERRTYPE_LAMBDA: {
    lselambda_t *lambda = lserref_get_lambda(erref);
    return lsthunk_new_expr(lsexpr_new_lambda(lambda));
  }
  }
}

lsthunk_t *lstref_apply(lstref_t *tref, const lstlist_t *args) {
  return NULL; // TODO: implement
}
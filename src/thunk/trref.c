#include "thunk/trref.h"
#include "common/malloc.h"

struct lstrref {
  const lsstr_t *ltrr_name;
  const lserref_t *ltrr_erref;
  lstenv_t *ltrr_env;
};

lstrref_t *lstrref_new(lstenv_t *tenv, const lsstr_t *name,
                       const lserref_t *erref) {
  lstrref_t *trref = lsmalloc(sizeof(lstrref_t));
  trref->ltrr_name = name;
  trref->ltrr_erref = erref;
  trref->ltrr_env = tenv;
  return trref;
}

lsthunk_t *lstrref_eval(lstrref_t *trref) {
  return NULL; // TODO: implement
}
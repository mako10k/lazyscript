#include "thunk/trref.h"
#include "common/malloc.h"

struct lstrref {
  const lsstr_t *name;
  const lserref_t *erref;
  lstenv_t *tenv;
};

lstrref_t *lstrref_new(lstenv_t *tenv, const lsstr_t *name,
                       const lserref_t *erref) {
  lstrref_t *trref = lsmalloc(sizeof(lstrref_t));
  trref->name = name;
  trref->erref = erref;
  trref->tenv = tenv;
  return trref;
}

lsthunk_t *lstrref_eval(lstrref_t *trref) {
  return NULL; // TODO: implement
}
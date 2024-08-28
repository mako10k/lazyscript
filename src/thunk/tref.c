#include "thunk/tref.h"
#include "common/malloc.h"
#include "thunk/trref.h"

struct lstref {
  lstrref_t *ltr_trref;
};

lstref_t *lstref_new(lstenv_t *tenv, const lseref_t *eref) {
  const lsstr_t *name = lseref_get_name(eref);
  lserref_t *erref = lseref_get_erref(eref);
  lstref_t *tref = lsmalloc(sizeof(lstref_t));
  tref->ltr_trref = lstrref_new(tenv, name, erref);
  return tref;
}

lsthunk_t *lstref_eval(lstref_t *tref) {
  return lstrref_eval(tref->ltr_trref); // TODO: implement
}
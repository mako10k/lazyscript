#include "tref.h"
#include "malloc.h"
#include "trref.h"

struct lstref {
  lstrref_t *ltrr_trref;
};

lstref_t *lstref_new(lstenv_t *tenv, const lseref_t *eref) {
  const lsstr_t *name = lseref_get_name(eref);
  lserref_t *erref = lseref_get_erref(eref);
  lstref_t *tref = lsmalloc(sizeof(lstref_t));
  tref->ltrr_trref = lstrref_new(tenv, name, erref);
  return tref;
}
#include "trref.h"
#include "malloc.h"

struct lstrref {
  const lsstr_t *name;
  const lserref_t *erref;
  lstenv_t *tenv;
};

lstrref_t *lstrref(lstenv_t *tenv, const lsstr_t *name,
                   const lserref_t *erref) {
  lstrref_t *trref = lsmalloc(sizeof(lstrref_t));
  trref->name = name;
  trref->erref = erref;
  trref->tenv = tenv;
  return trref;
}
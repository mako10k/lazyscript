#include "int.h"
#include "malloc.h"

struct lsint {
  int intval;
};

lsint_t *lsint(int intval) {
  lsint_t *eint = lsmalloc(sizeof(lsint_t));
  eint->intval = intval;
  return eint;
}
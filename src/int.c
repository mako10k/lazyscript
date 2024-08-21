#include "int.h"
#include "lazyscript.h"
#include "malloc.h"

struct lsint {
  int intval;
};

lsint_t *lsint(int intval) {
  lsint_t *eint = lsmalloc(sizeof(lsint_t));
  eint->intval = intval;
  return eint;
}

void lsint_print(FILE *fp, int prec, int indent, const lsint_t *intval) {
  (void)prec;
  (void)indent;
  lsprintf(fp, 0, "%d", intval->intval);
}
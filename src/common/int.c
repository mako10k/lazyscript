#include "common/int.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lsint {
  int li_val;
};

lsint_t *lsint(int val) {
  lsint_t *eint = lsmalloc(sizeof(lsint_t));
  eint->li_val = val;
  return eint;
}

void lsint_print(FILE *fp, lsprec_t prec, int indent, const lsint_t *val) {
  (void)prec;
  (void)indent;
  lsprintf(fp, 0, "%d", val->li_val);
}

int lsint_eq(const lsint_t *val1, const lsint_t *val2) {
  assert(val1 != NULL);
  assert(val2 != NULL);
  return val1 == val2 || val1->li_val == val2->li_val;
}
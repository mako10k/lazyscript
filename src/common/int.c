#include "common/int.h"
#include "common/io.h"
#include "common/malloc.h"
#include "lstypes.h"
#include <assert.h>

struct lsint {
  int li_val;
};

const lsint_t *lsint_new(int val) {
  if (val == 0)
    return NULL;
  lsint_t *eint = lsmalloc_atomic(sizeof(lsint_t));
  eint->li_val = val;
  return eint;
}

void lsint_print(FILE *fp, lsprec_t prec, int indent, const lsint_t *val) {
  (void)prec;
  (void)indent;
  assert(fp != NULL);
  assert(LSPREC_LOWEST <= prec && prec <= LSPREC_HIGHEST);
  assert(indent >= 0);
  int intval = val == NULL ? 0 : val->li_val;
  lsprintf(fp, 0, "%d", intval);
}

int lsint_eq(const lsint_t *restrict val1, const lsint_t *restrict val2) {
  if (val1 == val2)
    return 1;
  if (val1 == NULL)
    return val2->li_val == 0;
  if (val2 == NULL)
    return val1->li_val == 0;
  return val1->li_val == val2->li_val;
}

const lsint_t *lsint_add(const lsint_t *val1, const lsint_t *val2) {
  if (val1 == NULL)
    return val2;
  if (val2 == NULL)
    return val1;
  return lsint_new(val1->li_val + val2->li_val);
}

const lsint_t *lsint_sub(const lsint_t *val1, const lsint_t *val2) {
  if (val1 == NULL)
    return val2;
  if (val2 == NULL)
    return val1;
  return lsint_new(val1->li_val - val2->li_val);
}
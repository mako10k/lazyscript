#include "expr.h"

struct lsexpr {
  lsetype_t type;
  union {
    lsealge_t *ealge;
    lsappl_t *eappl;
    lseref_t *eref;
    const lsint_t *eint;
  };
};

lsexpr_t *lsexpr_alge(lsealge_t *ealge) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_ALGE;
  expr->ealge = ealge;
  return expr;
}

lsexpr_t *lsexpr_appl(lsappl_t *eappl) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_APPL;
  expr->eappl = eappl;
  return expr;
}

lsexpr_t *lsexpr_ref(lseref_t *eref) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_REF;
  expr->eref = eref;
  return expr;
}

lsexpr_t *lsexpr_int(const lsint_t *eint) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_INT;
  expr->eint = eint;
  return expr;
}
#include "expr.h"
#include "lazyscript.h"
#include <assert.h>

struct lsexpr {
  lsetype_t type;
  union {
    lsealge_t *ealge;
    lsappl_t *appl;
    lseref_t *eref;
    lslambda_t *lambda;
    const lsint_t *intval;
    const lsstr_t *strval;
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
  expr->appl = eappl;
  return expr;
}

lsexpr_t *lsexpr_ref(lseref_t *eref) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_REF;
  expr->eref = eref;
  return expr;
}

lsexpr_t *lsexpr_int(const lsint_t *intval) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_INT;
  expr->intval = intval;
  return expr;
}

lsexpr_t *lsexpr_str(const lsstr_t *strval) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_STR;
  expr->strval = strval;
  return expr;
}

lsexpr_t *lsexpr_lambda(lslambda_t *lambda) {
  lsexpr_t *expr = malloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_LAMBDA;
  expr->lambda = lambda;
  return expr;
}

void lsexpr_print(FILE *fp, int prec, int indent, const lsexpr_t *expr) {
#ifdef DEBUG
  if (expr == NULL) {
    lsprintf(fp, indent, "NULL");
    return;
  }
#else
  assert(expr != NULL);
#endif
  switch (expr->type) {
  case LSETYPE_ALGE:
    lsealge_print(fp, prec, indent, expr->ealge);
    break;
  case LSETYPE_APPL:
    lsappl_print(fp, prec, indent, expr->appl);
    break;
  case LSETYPE_REF:
    // TODO: Implement lseref_print
    break;
  case LSETYPE_INT:
    lsint_print(fp, prec, indent, expr->intval);
    break;
  case LSETYPE_STR:
    lsstr_print(fp, prec, indent, expr->strval);
    break;
  case LSETYPE_LAMBDA:
    lslambda_print(fp, prec, indent, expr->lambda);
    break;
  default:
    lsprintf(fp, indent, "Unknown expression type\n");
  }
}
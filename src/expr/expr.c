#include "expr/expr.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/echoice.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"
#include <assert.h>

struct lsexpr {
  lsetype_t le_type;
  union {
    const lsealge_t *le_alge;
    const lseappl_t *le_appl;
    const lsref_t *le_ref;
    const lselambda_t *le_lambda;
    const lseclosure_t *le_closure;
    const lsechoice_t *le_choice;
    const lsint_t *le_intval;
    const lsstr_t *le_strval;
  };
};

const lsexpr_t *lsexpr_new_alge(const lsealge_t *ealge) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_ALGE;
  expr->le_alge = ealge;
  return expr;
}

const lsexpr_t *lsexpr_new_appl(const lseappl_t *eappl) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_APPL;
  expr->le_appl = eappl;
  return expr;
}

const lsexpr_t *lsexpr_new_ref(const lsref_t *ref) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_REF;
  expr->le_ref = ref;
  return expr;
}

const lsexpr_t *lsexpr_new_int(const lsint_t *intval) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_INT;
  expr->le_intval = intval;
  return expr;
}

const lsexpr_t *lsexpr_new_str(const lsstr_t *strval) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_STR;
  expr->le_strval = strval;
  return expr;
}

const lsexpr_t *lsexpr_new_lambda(const lselambda_t *lambda) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_LAMBDA;
  expr->le_lambda = lambda;
  return expr;
}

const lsexpr_t *lsexpr_new_choice(const lsechoice_t *echoice) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_CHOICE;
  expr->le_choice = echoice;
  return expr;
}

const lsexpr_t *lsexpr_new_closure(const lseclosure_t *closure) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_CLOSURE;
  expr->le_closure = closure;
  return expr;
}

lsetype_t lsexpr_get_type(const lsexpr_t *expr) { return expr->le_type; }

const lsealge_t *lsexpr_get_alge(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_ALGE);
  return expr->le_alge;
}

const lseappl_t *lsexpr_get_appl(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_APPL);
  return expr->le_appl;
}

const lsref_t *lsexpr_get_ref(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_REF);
  return expr->le_ref;
}

const lsint_t *lsexpr_get_int(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_INT);
  return expr->le_intval;
}

const lsstr_t *lsexpr_get_str(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_STR);
  return expr->le_strval;
}

const lselambda_t *lsexpr_get_lambda(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_LAMBDA);
  return expr->le_lambda;
}

const lseclosure_t *lsexpr_get_closure(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_CLOSURE);
  return expr->le_closure;
}

const lsechoice_t *lsexpr_get_choice(const lsexpr_t *expr) {
  assert(expr->le_type == LSETYPE_CHOICE);
  return expr->le_choice;
}

void lsexpr_print(FILE *fp, lsprec_t prec, int indent, const lsexpr_t *expr) {
  switch (expr->le_type) {
  case LSETYPE_ALGE:
    lsealge_print(fp, prec, indent, expr->le_alge);
    return;
  case LSETYPE_APPL:
    lseappl_print(fp, prec, indent, expr->le_appl);
    return;
  case LSETYPE_REF:
    lsref_print(fp, prec, indent, expr->le_ref);
    return;
  case LSETYPE_INT:
    lsint_print(fp, prec, indent, expr->le_intval);
    return;
  case LSETYPE_STR:
    lsstr_print(fp, prec, indent, expr->le_strval);
    return;
  case LSETYPE_LAMBDA:
    lselambda_print(fp, prec, indent, expr->le_lambda);
    return;
  case LSETYPE_CLOSURE:
    lseclosure_print(fp, prec, indent, expr->le_closure);
    return;
  case LSETYPE_CHOICE:
    lsechoice_print(fp, prec, indent, expr->le_choice);
    return;
  }
  lsprintf(fp, indent, "Unknown expression type %d\n", expr->le_type);
}

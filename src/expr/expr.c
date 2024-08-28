#include "expr/expr.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/echoice.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"
#include "expr/eref.h"
#include <assert.h>

struct lsexpr {
  lsetype_t le_type;
  union {
    lsealge_t *le_alge;
    lseappl_t *le_appl;
    lseref_t *le_ref;
    lselambda_t *le_lambda;
    lseclosure_t *le_closure;
    lsechoice_t *le_choice;
    const lsint_t *le_intval;
    const lsstr_t *le_strval;
  };
};

lsexpr_t *lsexpr_new_alge(lsealge_t *ealge) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_ALGE;
  expr->le_alge = ealge;
  return expr;
}

lsexpr_t *lsexpr_new_appl(lseappl_t *eappl) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_APPL;
  expr->le_appl = eappl;
  return expr;
}

lsexpr_t *lsexpr_new_ref(lseref_t *eref) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_REF;
  expr->le_ref = eref;
  return expr;
}

lsexpr_t *lsexpr_new_int(const lsint_t *intval) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_INT;
  expr->le_intval = intval;
  return expr;
}

lsexpr_t *lsexpr_new_str(const lsstr_t *strval) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_STR;
  expr->le_strval = strval;
  return expr;
}

lsexpr_t *lsexpr_new_lambda(lselambda_t *lambda) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_LAMBDA;
  expr->le_lambda = lambda;
  return expr;
}

lsexpr_t *lsexpr_new_choice(lsechoice_t *echoice) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_CHOICE;
  expr->le_choice = echoice;
  return expr;
}

void lsexpr_print(FILE *fp, lsprec_t prec, int indent, const lsexpr_t *expr) {
#ifdef DEBUG
  if (expr == NULL) {
    lsprintf(fp, indent, "NULL");
    return;
  }
#else
  assert(expr != NULL);
#endif
  switch (expr->le_type) {
  case LSETYPE_ALGE:
    lsealge_print(fp, prec, indent, expr->le_alge);
    return;
  case LSETYPE_APPL:
    lseappl_print(fp, prec, indent, expr->le_appl);
    return;
  case LSETYPE_REF:
    lseref_print(fp, prec, indent, expr->le_ref);
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
  lsprintf(fp, indent, "Unknown expression type\n");
}

lsexpr_t *lsexpr_new_closure(lseclosure_t *closure) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type = LSETYPE_CLOSURE;
  expr->le_closure = closure;
  return expr;
}

lsetype_t lsexpr_get_type(const lsexpr_t *expr) {
  assert(expr != NULL);
  return expr->le_type;
}

lsealge_t *lsexpr_get_alge(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_ALGE);
  return expr->le_alge;
}

lseappl_t *lsexpr_get_appl(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_APPL);
  return expr->le_appl;
}

lseref_t *lsexpr_get_ref(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_REF);
  return expr->le_ref;
}

const lsint_t *lsexpr_get_int(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_INT);
  return expr->le_intval;
}

const lsstr_t *lsexpr_get_str(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_STR);
  return expr->le_strval;
}

lselambda_t *lsexpr_get_lambda(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_LAMBDA);
  return expr->le_lambda;
}

lseclosure_t *lsexpr_get_closure(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_CLOSURE);
  return expr->le_closure;
}

lsechoice_t *lsexpr_get_choice(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->le_type == LSETYPE_CHOICE);
  return expr->le_choice;
}

lspres_t lsexpr_prepare(lsexpr_t *const expr, lseenv_t *const env) {
  assert(expr != NULL);
  assert(env != NULL);
  switch (expr->le_type) {
  case LSETYPE_ALGE:
    return lsealge_prepare(expr->le_alge, env);
  case LSETYPE_APPL:
    return lseappl_prepare(expr->le_appl, env);
  case LSETYPE_REF:
    return lseref_prepare(expr->le_ref, env);
  case LSETYPE_LAMBDA:
    return lselambda_prepare(expr->le_lambda, env);
  case LSETYPE_CLOSURE:
    return lseclosure_prepare(expr->le_closure, env);
  case LSETYPE_CHOICE:
    return lsechoice_prepare(expr->le_choice, env);
  case LSETYPE_INT:
  case LSETYPE_STR:
    return LSPRES_SUCCESS;
  }
  assert(0);
}
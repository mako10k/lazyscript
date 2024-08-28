#include "expr/expr.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"
#include "expr/eref.h"
#include "thunk/talge.h"
#include "thunk/tappl.h"
#include "thunk/tref.h"
#include <assert.h>

struct lsexpr {
  lsetype_t type;
  union {
    lsealge_t *ealge;
    lseappl_t *appl;
    lseref_t *eref;
    lselambda_t *lambda;
    lseclosure_t *closure;
    const lsint_t *intval;
    const lsstr_t *strval;
  };
};

lsexpr_t *lsexpr_new_alge(lsealge_t *ealge) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_ALGE;
  expr->ealge = ealge;
  return expr;
}

lsexpr_t *lsexpr_new_appl(lseappl_t *eappl) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_APPL;
  expr->appl = eappl;
  return expr;
}

lsexpr_t *lsexpr_new_ref(lseref_t *eref) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_REF;
  expr->eref = eref;
  return expr;
}

lsexpr_t *lsexpr_new_int(const lsint_t *intval) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_INT;
  expr->intval = intval;
  return expr;
}

lsexpr_t *lsexpr_new_str(const lsstr_t *strval) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_STR;
  expr->strval = strval;
  return expr;
}

lsexpr_t *lsexpr_new_lambda(lselambda_t *lambda) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_LAMBDA;
  expr->lambda = lambda;
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
  switch (expr->type) {
  case LSETYPE_ALGE:
    lsealge_print(fp, prec, indent, expr->ealge);
    break;
  case LSETYPE_APPL:
    lseappl_print(fp, prec, indent, expr->appl);
    break;
  case LSETYPE_REF:
    lseref_print(fp, prec, indent, expr->eref);
    break;
  case LSETYPE_INT:
    lsint_print(fp, prec, indent, expr->intval);
    break;
  case LSETYPE_STR:
    lsstr_print(fp, prec, indent, expr->strval);
    break;
  case LSETYPE_LAMBDA:
    lselambda_print(fp, prec, indent, expr->lambda);
    break;
  case LSETYPE_CLOSURE:
    lseclosure_print(fp, prec, indent, expr->closure);
    break;
  default:
    lsprintf(fp, indent, "Unknown expression type\n");
  }
}

lsexpr_t *lsexpr_new_closure(lseclosure_t *closure) {
  lsexpr_t *expr = lsmalloc(sizeof(lsexpr_t));
  expr->type = LSETYPE_CLOSURE;
  expr->closure = closure;
  return expr;
}

lsetype_t lsexpr_get_type(const lsexpr_t *expr) {
  assert(expr != NULL);
  return expr->type;
}

lsealge_t *lsexpr_get_alge(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_ALGE);
  return expr->ealge;
}

lseappl_t *lsexpr_get_appl(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_APPL);
  return expr->appl;
}

lseref_t *lsexpr_get_ref(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_REF);
  return expr->eref;
}

const lsint_t *lsexpr_get_int(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_INT);
  return expr->intval;
}

const lsstr_t *lsexpr_get_str(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_STR);
  return expr->strval;
}

lselambda_t *lsexpr_get_lambda(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_LAMBDA);
  return expr->lambda;
}

lseclosure_t *lsexpr_get_closure(const lsexpr_t *expr) {
  assert(expr != NULL);
  assert(expr->type == LSETYPE_CLOSURE);
  return expr->closure;
}

lspres_t lsexpr_prepare(lsexpr_t *const expr, lseenv_t *const env) {
  assert(expr != NULL);
  assert(env != NULL);
  switch (expr->type) {
  case LSETYPE_ALGE:
    return lsealge_prepare(expr->ealge, env);
  case LSETYPE_APPL:
    return lseappl_prepare(expr->appl, env);
  case LSETYPE_REF:
    return lseref_prepare(expr->eref, env);
  case LSETYPE_LAMBDA:
    return lselambda_prepare(expr->lambda, env);
  case LSETYPE_CLOSURE:
    return lseclosure_prepare(expr->closure, env);
  default:
    return LSPRES_SUCCESS;
  }
}

lsthunk_t *lsexpr_thunk(lstenv_t *tenv, const lsexpr_t *expr) {
  assert(expr != NULL);
  switch (expr->type) {
  case LSETYPE_ALGE:
    return lsthunk_alge(lstalge_new(tenv, expr->ealge));
  case LSETYPE_APPL:
    return lsthunk_appl(lstappl_new(tenv, expr->appl));
  case LSETYPE_REF:
    return lsthunk_ref(lstref_new(tenv, expr->eref));
  case LSETYPE_LAMBDA:
    return lsthunk_lambda(lstlambda_new(tenv, expr->lambda));
  case LSETYPE_CLOSURE:
    return lsthunk_closure(lstclosure_new(tenv, expr->closure));
  case LSETYPE_INT:
    return lsthunk_int(expr->intval);
  case LSETYPE_STR:
    return lsthunk_str(expr->strval);
  }
  assert(0);
}

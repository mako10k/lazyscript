#include "thunk.h"
#include "expr.h"
#include "malloc.h"
#include "talge.h"
#include "tclosure.h"
#include "tlambda.h"
#include <assert.h>

struct lsthunk {
  lsttype_t type;
  union {
    lstalge_t *talge;
    lstappl_t *tappl;
    const lsint_t *intval;
    const lsstr_t *strval;
    lstref_t *tref;
    lstlambda_t *tlambda;
    lstclosure_t *tclosure;
  };
  lsthunk_t *thunk_whnf;
};

lsthunk_t *lsthunk_appl(lstappl_t *tappl) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_APPL;
  thunk->tappl = tappl;
  thunk->thunk_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_alge(lstalge_t *talge) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_ALGE;
  thunk->talge = talge;
  thunk->thunk_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_int(const lsint_t *intval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_INT;
  thunk->intval = intval;
  thunk->thunk_whnf = thunk;
  return thunk;
}

lsthunk_t *lsthunk_str(const lsstr_t *strval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_STR;
  thunk->strval = strval;
  thunk->thunk_whnf = thunk;
  return thunk;
}

lsthunk_t *lsthunk_ref(lstref_t *tref) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_REF;
  thunk->tref = tref;
  thunk->thunk_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_lambda(lstlambda_t *tlambda) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_LAMBDA;
  thunk->tlambda = tlambda;
  thunk->thunk_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_closure(lstclosure_t *tclosure) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->type = LSTTYPE_CLOSURE;
  thunk->tclosure = tclosure;
  thunk->thunk_whnf = NULL;
  return thunk;
}
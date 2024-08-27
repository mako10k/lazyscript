#include "thunk.h"
#include "malloc.h"
#include "talge.h"
#include "tclosure.h"
#include "tlambda.h"
#include <assert.h>

struct lsthunk {
  lsttype_t lt_type;
  union {
    lstalge_t *lt_alge;
    lstappl_t *lt_appl;
    const lsint_t *lt_int;
    const lsstr_t *lt_str;
    lstref_t *lt_ref;
    lstlambda_t *lt_lambda;
    lstclosure_t *lt_closure;
  };
  lsthunk_t *lt_whnf;
};

lsthunk_t *lsthunk_appl(lstappl_t *tappl) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_APPL;
  thunk->lt_appl = tappl;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_alge(lstalge_t *talge) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_ALGE;
  thunk->lt_alge = talge;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_int(const lsint_t *intval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_INT;
  thunk->lt_int = intval;
  thunk->lt_whnf = thunk;
  return thunk;
}

lsthunk_t *lsthunk_str(const lsstr_t *strval) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_STR;
  thunk->lt_str = strval;
  thunk->lt_whnf = thunk;
  return thunk;
}

lsthunk_t *lsthunk_ref(lstref_t *tref) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_REF;
  thunk->lt_ref = tref;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_lambda(lstlambda_t *tlambda) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_LAMBDA;
  thunk->lt_lambda = tlambda;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_closure(lstclosure_t *tclosure) {
  lsthunk_t *thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type = LSTTYPE_CLOSURE;
  thunk->lt_closure = tclosure;
  thunk->lt_whnf = NULL;
  return thunk;
}

lsthunk_t *lsthunk_get_whnf(lsthunk_t *thunk) {
  assert(thunk != NULL);
  if (thunk->lt_whnf != NULL)
    return thunk->lt_whnf;

  switch (thunk->lt_type) {
  case LSTTYPE_APPL:
    thunk->lt_whnf = lstappl_eval(thunk->lt_appl);
    break;
  case LSTTYPE_CLOSURE:
    thunk->lt_whnf = lstclosure_eval(thunk->lt_closure);
    break;
  case LSTTYPE_REF:
    thunk->lt_whnf = lstref_eval(thunk->lt_ref);
    break;
  default:
    // it already is in WHNF
    thunk->lt_whnf = thunk;
    break;
  }
  return thunk->lt_whnf;
}

lsttype_t lsthunk_type(const lsthunk_t *thunk) { return thunk->lt_type; }

lstalge_t *lsthunk_get_alge(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge;
}
lstappl_t *lsthunk_get_appl(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_appl;
}
const lsint_t *lsthunk_get_int(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}
const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}
lstclosure_t *lsthunk_get_closure(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_CLOSURE);
  return thunk->lt_closure;
}
lstlambda_t *lsthunk_get_lambda(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda;
}
lstref_t *lsthunk_get_ref(const lsthunk_t *thunk) {
  assert(thunk->lt_type == LSTTYPE_REF);
  return thunk->lt_ref;
}
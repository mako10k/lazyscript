#include "thunk/tappl.h"
#include "common/io.h"
#include "common/malloc.h"
#include "thunk/thunk.h"
#include "thunk/tlist.h"
#include <assert.h>

struct lstappl {
  lsthunk_t *lta_func;
  const lstlist_t *lta_args;
};

lstappl_t *lstappl_new(lstenv_t *tenv, const lseappl_t *eappl) {
  assert(tenv != NULL);
  assert(eappl != NULL);
  lstappl_t *tappl = lsmalloc(sizeof(lstappl_t));
  tappl->lta_func = lsthunk_new_expr(tenv, lseappl_get_func(eappl));
  tappl->lta_args = lstlist_new();
  for (const lselist_t *le = lseappl_get_args(eappl); le != NULL;
       le = lselist_get_next(le)) {
    lsexpr_t *arg = lselist_get(le, 0);
    lstlist_push(tappl->lta_args, lsthunk_new_expr(tenv, arg));
  }
  return tappl;
}

lsthunk_t *lstappl_apply(lstappl_t *tappl, const lstlist_t *args) {
  assert(tappl != NULL);
  assert(args != NULL);
  lstappl_t *tappl2 = lsmalloc(sizeof(lstappl_t));
  tappl2->lta_func = tappl->lta_func;
  tappl2->lta_args = lstlist_concat(tappl->lta_args, args);
  return lsthunk_appl(tappl2);
}

lsthunk_t *lstappl_eval(lstappl_t *tappl) {
  assert(tappl != NULL);
  lsthunk_t *func = lsthunk_get_whnf(tappl->lta_func);
  if (lstlist_count(tappl->lta_args) == 0)
    return func;
  lsttype_t ttype = lsthunk_type(func);
  assert(ttype != LSTTYPE_REF);
  switch (ttype) {
  case LSTTYPE_APPL:
    return lstappl_apply(lsthunk_get_appl(func), tappl->lta_args);
  case LSTTYPE_ALGE:
    return lstalge_apply(lsthunk_get_alge(func), tappl->lta_args);
  case LSTTYPE_CLOSURE:
    return lstclosure_apply(lsthunk_get_closure(func), tappl->lta_args);
  case LSTTYPE_LAMBDA:
    return lstlambda_apply(lsthunk_get_lambda(func), tappl->lta_args);
  case LSTTYPE_INT:
    lsprintf(stderr, 0, "F: cannot apply for integer\n");
    exit(1);
  case LSTTYPE_STR:
    lsprintf(stderr, 0, "F: cannot apply for string\n");
    exit(1);
  case LSTTYPE_REF:
    assert(0);
  }
  assert(0);
}
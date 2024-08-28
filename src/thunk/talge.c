#include "thunk/talge.h"
#include "common/malloc.h"
#include "expr/ealge.h"
#include "thunk/tlist.h"
#include <assert.h>

struct lstalge {
  const lsstr_t *ltag_constr;
  const lstlist_t *ltag_args;
};

lstalge_t *lstalge_new(lstenv_t *tenv, const lsealge_t *ealge) {
  lstalge_t *alge = lsmalloc(sizeof(lstalge_t));
  alge->ltag_constr = lsealge_get_constr(ealge);
  alge->ltag_args = lstlist_new();
  for (const lselist_t *le = lsealge_get_args(ealge); le != NULL;
       le = lselist_get_next(le)) {
    lsexpr_t *arg = lselist_get(le, 0);
    lstlist_push(alge->ltag_args, lsthunk_new_expr(tenv, arg));
  }
  return alge;
}

lsthunk_t *lstalge_apply(lstalge_t *talge, const lstlist_t *args) {
  assert(talge != NULL);
  talge->ltag_constr = talge->ltag_constr;
  talge->ltag_args = lstlist_concat(talge->ltag_args, args);
  return lsthunk_alge(talge);
}
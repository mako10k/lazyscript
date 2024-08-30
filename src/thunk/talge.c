#include "thunk/talge.h"
#include "common/malloc.h"
#include "expr/ealge.h"
#include "thunk/tlist.h"
#include <assert.h>

struct lstalge {
  const lsstr_t *lta_constr;
  lssize_t lta_argc;
  const lsthunk_t *lta_args[0];
};

lstalge_t *lstalge_new(const lsealge_t *ealge, lstenv_t *tenv) {
  size_t eargc = lsealge_get_argc(ealge);
  const lsexpr_t *const *eargs = lsealge_get_args(ealge);
  lstalge_t *talge = lsmalloc(sizeof(lstalge_t) + eargc * sizeof(lsthunk_t *));
  talge->lta_constr = lsealge_get_constr(ealge);
  talge->lta_argc = lsealge_get_argc(ealge);
  for (size_t i = 0; i < eargc; i++)
    talge->lta_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return talge;
}

lsthunk_t *lstalge_apply(lstalge_t *talge, size_t argc, const lsthunk_t *args[]) {
  size_t argc0 = talge->lta_argc;
  lstalge_t *talge0 = lsmalloc(sizeof(lstalge_t) + (argc0 + argc) * sizeof(lsthunk_t *));
  talge->lta_constr = talge->lta_constr;
  for (size_t i = 0; i < argc0; i++)
    talge0->lta_args[i] = talge->lta_args[i];
  for (size_t i = 0; i < argc; i++)
    talge0->lta_args[argc0 + i] = args[i];
  return lsthunk_new_alge(talge0);
}

const lsstr_t *lstalge_get_constr(const lstalge_t *talge) {
  assert(talge != NULL);
  return talge->lta_constr;
}

lssize_t lstalge_get_arg_count(const lstalge_t *talge) {
  assert(talge != NULL);
  return lstlist_count(talge->lta_args);
}

lsthunk_t *lstalge_get_arg(const lstalge_t *talge, int i) {
  assert(talge != NULL);
  return lstlist_get(talge->lta_args, i);
}

const lstlist_t *lstalge_get_args(const lstalge_t *talge) {
  assert(talge != NULL);
  return talge->lta_args;
}
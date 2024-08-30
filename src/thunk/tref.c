#include "thunk/tref.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "misc/bind.h"
#include "thunk/thunk.h"
#include <assert.h>

struct lstref_bind_entry {
  lstenv_t *ltrb_env;
  const lspat_t *ltrb_lhs;
  lsthunk_t *ltrb_rhs;
};

struct lstref_lambda {
  const lspat_t *ltrl_arg;
  lsthunk_t *ltrl_body;
};

struct lstref_target {
  lstrtype_t ltrt_type;
  union {
    const lstref_bind_entry_t *ltrt_bind_entry;
    const lstref_lambda_t *ltrt_lambda;
    lsthunk_t *ltrt_thunk;
  };
  const lstref_entry_t *ltrt_entry;
};

struct lstref_entry {
  const lsref_t *ltre_ref;
  lsthunk_t *ltre_bound;
  const lstref_entry_t *ltre_next;
};

struct lstref {
  const lstref_target_t *ltr_target;
  const lstref_entry_t *ltr_entry;
};

lstref_t *lstref_new_thunk(const lsref_t *ref, lsthunk_t *thunk) {
  assert(ref != NULL);
  assert(thunk != NULL);
  lstref_t *tref = lsmalloc(sizeof(lstref_t));
  lstref_target_t *target = lsmalloc(sizeof(lstref_target_t));
  target->ltrt_type = LSTRTYPE_THUNK;
  target->ltrt_thunk = thunk;
  tref->ltr_target = target;
  return tref;
}

lstref_t *lstref_new_bind_entry(const lsbind_entry_t *bentry, lstenv_t *tenv) {
  assert(bentry != NULL);
  assert(tenv != NULL);
  lstref_t *tref = lsmalloc(sizeof(lstref_t));
  lstref_target_t *target = lsmalloc(sizeof(lstref_target_t));
  target->ltrt_type = LSTRTYPE_BIND_ENTRY;
  lstref_bind_entry_t *trbentry = lsmalloc(sizeof(lstref_bind_entry_t));
  trbentry->ltrb_env = tenv;
  trbentry->ltrb_lhs = lsbind_entry_get_lhs(bentry);
  trbentry->ltrb_rhs = lsthunk_new_expr(lsbind_entry_get_rhs(bentry), tenv);
  target->ltrt_bind_entry = trbentry;
  return tref;
}

lstref_t *lstref_new(const lsref_t *ref, lstenv_t *tenv) {
  assert(ref != NULL);
  assert(tenv != NULL);
  const lsstr_t *ename = lsref_get_name(ref);
  return lstenv_get(tenv, ename);
}

const lsthunk_t *lstref_bind_entry_eval(const lstref_bind_entry_t *bentry) {
  assert(bentry != NULL);
  lsmres_t mres  = lsthunk_match_pat(bentry->ltrb_rhs, bentry->ltrb_lhs, tenv);
  return bentry->ltrb_rhs;
}

lsthunk_t *lstref_eval(lstref_t *tref) {
  assert(tref != NULL);
  switch (tref->ltr_target->ltrt_type) {
  case LSTRTYPE_THUNK:
    return tref->ltr_target->ltrt_thunk;
  case LSTRTYPE_BIND_ENTRY:
    return lstref_bind_entry_eval(tref->ltr_target->ltrt_bind_entry);
  case LSTRTYPE_LAMBDA:
    return lstref_lambda_eval(tref->ltr_target->ltrt_lambda);
  }
}

lsthunk_t *lstref_apply(lstref_t *tref, const lstlist_t *args) {
  return NULL; // TODO: implement
}

const lstref_entry_t *lstref_entry_add_pat(const lstref_entry_t *entry,
                                           const lspat_t *pat, lstenv_t *tenv);

const lstref_entry_t *lstref_entry_add_palge(const lstref_entry_t *entry,
                                             const lspalge_t *alge,
                                             lstenv_t *tenv) {
  assert(alge != NULL);
  assert(tenv != NULL);
  const lstref_entry_t **pentry = &entry;
  for (const lsplist_t *arg = lspalge_get_args(alge); arg != NULL;
       arg = lsplist_get_next(arg)) {
    const lspat_t *pat = lsplist_get(arg, 0);
    entry = lstref_entry_add_pat(entry, pat, tenv);
  }
  return entry;
}

const lstref_entry_t *lstref_entry_add_ref(const lstref_entry_t *entry,
                                           const lsref_t *ref, lstenv_t *tenv) {
  assert(ref != NULL);
  assert(tenv != NULL);
  lstref_entry_t *new_entry = lsmalloc(sizeof(lstref_entry_t));
  new_entry->ltre_ref = ref;
  new_entry->ltre_bound = NULL;
  new_entry->ltre_next = entry;
  return new_entry;
}

const lstref_entry_t *lstref_entry_add_pas(const lstref_entry_t *entry,
                                           const lspas_t *pas, lstenv_t *tenv) {
  assert(pas != NULL);
  assert(tenv != NULL);
  const lspat_t *pat = lspas_get_pat(pas);
  const lsref_t *ref = lspas_get_ref(pas);
  return lstref_entry_add_ref(lstref_entry_add_pat(entry, pat, tenv), ref,
                              tenv);
  if (entry == NULL)
    return NULL;
  return entry;
}

const lstref_entry_t *lstref_entry_add_pat(const lstref_entry_t *entry,
                                           const lspat_t *pat, lstenv_t *tenv) {
  assert(pat != NULL);
  assert(tenv != NULL);
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE:
    return lstref_entry_add_palge(entry, lspat_get_alge(pat), tenv);
  case LSPTYPE_AS:
    return lstref_target_create_entries_as(lspat_get_as(pat), tenv);
  case LSPTYPE_INT:
    return lstref_target_create_entries_int(lspat_get_int(pat), tenv);
  case LSPTYPE_STR:
    return lstref_target_create_entries_str(lspat_get_str(pat), tenv);
  case LSPTYPE_REF:
    return lstref_target_create_entries_ref(lspat_get_ref(pat), tenv);
  }
}

const lstref_target_t *lstref_target_new_lambda(const lselambda_t *elambda,
                                                lstenv_t *tenv) {
  assert(elambda != NULL);
  assert(tenv != NULL);
  const lspat_t *arg = lselambda_get_arg(elambda);
  const lstref_entry_t *entry = lstref_target_create_entries(arg, tenv);
  lstref_target_t *target = lsmalloc(sizeof(lstref_target_t));
  target->ltrt_type = LSTRTYPE_LAMBDA;
  lstref_lambda_t *lambda = lsmalloc(sizeof(lstref_lambda_t));
  lambda->ltrl_arg = arg;
  lambda->ltrl_body = lsthunk_new_expr(lselambda_get_body(elambda), tenv);
  target->ltrt_lambda = lambda;
  return target;
}
#include "expr/erref.h"
#include "common/malloc.h"
#include "expr/elambda.h"
#include "misc/bind.h"
#include <assert.h>

struct lserref {
  lserrtype_t lerr_type;
  lspref_t *lerr_pref;
  union {
    lsbind_entry_t *lerr_bind_entry;
    lselambda_t *lerr_lambda;
  };
};

struct lserref_wrapper {
  lserrtype_t lerrw_type;
  union {
    lsbind_entry_t *lerrw_bind_entry;
    lselambda_t *lerrw_lambda;
  };
};

lserref_wrapper_t *lserref_wrapper_bind_ent(lsbind_entry_t *bentry) {
  assert(bentry != NULL);
  lserref_wrapper_t *const erref_wrapper = lsmalloc(sizeof(lserref_wrapper_t));
  erref_wrapper->lerrw_type = LSERRTYPE_BINDING;
  erref_wrapper->lerrw_bind_entry = bentry;
  return erref_wrapper;
}

lserref_wrapper_t *lserref_wrapper_lambda_ent(lselambda_t *lentry) {
  assert(lentry != NULL);
  lserref_wrapper_t *const erref_wrapper = lsmalloc(sizeof(lserref_wrapper_t));
  erref_wrapper->lerrw_type = LSERRTYPE_LAMBDA;
  erref_wrapper->lerrw_lambda = lentry;
  return erref_wrapper;
}

lserref_t *lserref_new(lserref_wrapper_t *erref_wrapper, lspref_t *pref) {
  assert(erref_wrapper != NULL);
  assert(pref != NULL);
  lserref_t *const erref = lsmalloc(sizeof(lserref_t));
  erref->lerr_type = erref_wrapper->lerrw_type;
  erref->lerr_pref = pref;
  switch (erref->lerr_type) {
  case LSERRTYPE_BINDING:
    erref->lerr_bind_entry = erref_wrapper->lerrw_bind_entry;
    return erref;
  case LSERRTYPE_LAMBDA:
    erref->lerr_lambda = erref_wrapper->lerrw_lambda;
    return erref;
  }
  assert(0);
}

lspref_t *lserref_get_pref(const lserref_t *erref) {
  assert(erref != NULL);
  return erref->lerr_pref;
}
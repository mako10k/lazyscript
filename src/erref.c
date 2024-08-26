#include "erref.h"
#include "bind.h"
#include "elambda.h"
#include "malloc.h"
#include <assert.h>

struct lserref {
  lserrtype_t type;
  lspref_t *pref;
  union {
    lsbind_ent_t *bind_ent;
    lselambda_ent_t *lambda_ent;
  };
};

struct lserref_wrapper {
  lserrtype_t type;
  union {
    lsbind_ent_t *bind_ent;
    lselambda_ent_t *lambda_ent;
  };
};

lserref_wrapper_t *lserref_wrapper_bind_ent(lsbind_ent_t *ent) {
  assert(ent != NULL);
  lserref_wrapper_t *const erref_wrapper = lsmalloc(sizeof(lserref_wrapper_t));
  erref_wrapper->type = LSERRTYPE_BINDING;
  erref_wrapper->bind_ent = ent;
  return erref_wrapper;
}

lserref_wrapper_t *lserref_wrapper_lambda_ent(lselambda_ent_t *ent) {
  assert(ent != NULL);
  lserref_wrapper_t *const erref_wrapper = lsmalloc(sizeof(lserref_wrapper_t));
  erref_wrapper->type = LSERRTYPE_LAMBDA;
  erref_wrapper->lambda_ent = ent;
  return erref_wrapper;
}

lserref_t *lserref(lserref_wrapper_t *erref_wrapper, lspref_t *pref) {
  assert(erref_wrapper != NULL);
  assert(pref != NULL);
  lserref_t *const erref = lsmalloc(sizeof(lserref_t));
  erref->type = erref_wrapper->type;
  erref->pref = pref;
  switch (erref->type) {
  case LSERRTYPE_BINDING:
    erref->bind_ent = erref_wrapper->bind_ent;
    break;
  case LSERRTYPE_LAMBDA:
    erref->lambda_ent = erref_wrapper->lambda_ent;
    break;
  default:
    break;
  }
  return erref;
}

lspref_t *lserref_get_pref(const lserref_t *erref) {
  assert(erref != NULL);
  return erref->pref;
}

static lsthunk_t *lsbind_ent_thunk(const lsbind_ent_t *ent) {
  assert(ent != NULL);
  
  return NULL;
}

static lsthunk_t *lselambda_ent_thunk(const lselambda_ent_t *ent) {
  assert(ent != NULL);
  return NULL;
}

lsthunk_t *lserref_thunk(const lserref_t *erref) {
  assert(erref != NULL);
  switch (erref->type) {
  case LSERRTYPE_BINDING:
    return lsbind_ent_thunk(erref->bind_ent);
  case LSERRTYPE_LAMBDA:
    return lselambda_ent_thunk(erref->lambda_ent);
  default:
    assert(0);
  }
}

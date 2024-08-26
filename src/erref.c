#include "erref.h"
#include "bind.h"
#include "lambda.h"
#include "malloc.h"

struct lserref {
  lserrtype_t type;
  lspref_t *pref;
  union {
    lsbind_ent_t *bind_ent;
    lslambda_ent_t *lambda_ent;
  };
};

struct lserref_wrapper {
  lserrtype_t type;
  union {
    lsbind_ent_t *bind_ent;
    lslambda_ent_t *lambda_ent;
  };
};

lserref_wrapper_t *lserref_wrapper_bind_ent(lsbind_ent_t *ent) {
  lserref_wrapper_t *const erref_wrapper = lsmalloc(sizeof(lserref_wrapper_t));
  erref_wrapper->type = LSERRTYPE_BINDING;
  erref_wrapper->bind_ent = ent;
  return erref_wrapper;
}

lserref_wrapper_t *lserref_wrapper_lambda_ent(lslambda_ent_t *ent) {
  lserref_wrapper_t *const erref_wrapper = lsmalloc(sizeof(lserref_wrapper_t));
  erref_wrapper->type = LSERRTYPE_LAMBDA;
  erref_wrapper->lambda_ent = ent;
  return erref_wrapper;
}

lserref_t *lserref(lserref_wrapper_t *erref_wrapper, lspref_t *pref) {
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

lspref_t *lserref_get_pref(const lserref_t *erref) { return erref->pref; }

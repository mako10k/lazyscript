#include "erref.h"
#include "bind.h"
#include "lambda.h"

struct lserref {
  lserrtype_t type;
  union {
    lsbind_ent_t *bind_ent;
    lslambda_ent_t *lambda_ent;
  };
};

lserref_t *lserref_bind_ent(lsbind_ent_t *ent) {
  lserref_t *const erref = malloc(sizeof(lserref_t));
  erref->type = LSERRTYPE_BINDING;
  erref->bind_ent = ent;
  return erref;
}

lserref_t *lserref_lambda_ent(lslambda_ent_t *ent) {
  lserref_t *const erref = malloc(sizeof(lserref_t));
  erref->type = LSERRTYPE_LAMBDA;
  erref->lambda_ent = ent;
  return erref;
}
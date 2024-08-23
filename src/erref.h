#pragma once

typedef struct lserref lserref_t;

#include "bind.h"
#include "lambda.h"

lserref_t *lserref_bind_ent(lsbind_ent_t *ent);
lserref_t *lserref_lambda_ent(lslambda_ent_t *ent);

typedef enum {
  LSERRTYPE_VOID = 0,
  LSERRTYPE_BINDING,
  LSERRTYPE_LAMBDA,
} lserrtype_t;

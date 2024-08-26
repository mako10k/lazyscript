#pragma once

typedef struct lserref_wrapper lserref_wrapper_t;
typedef struct lserref lserref_t;

#include "bind.h"
#include "lambda.h"
#include "pref.h"

lserref_wrapper_t *lserref_wrapper_bind_ent(lsbind_ent_t *ent);
lserref_wrapper_t *lserref_wrapper_lambda_ent(lslambda_ent_t *ent);
lserref_t *lserref(lserref_wrapper_t *erref, lspref_t *pref);
lspref_t *lserref_get_pref(const lserref_t *erref);

typedef enum {
  LSERRTYPE_VOID = 0,
  LSERRTYPE_BINDING,
  LSERRTYPE_LAMBDA,
} lserrtype_t;

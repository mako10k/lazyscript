#pragma once

typedef struct lserref_wrapper lserref_wrapper_t;
typedef struct lserref lserref_t;

typedef enum {
  LSERRTYPE_BINDING,
  LSERRTYPE_LAMBDA,
} lserrtype_t;

#include "bind.h"
#include "elambda.h"
#include "pref.h"

lserref_wrapper_t *lserref_wrapper_bind_ent(lsbind_entry_t *ent);
lserref_wrapper_t *lserref_wrapper_lambda_ent(lselambda_entry_t *ent);
lserref_t *lserref_new(lserref_wrapper_t *erref, lspref_t *pref);
lspref_t *lserref_get_pref(const lserref_t *erref);
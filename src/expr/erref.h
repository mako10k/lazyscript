#pragma once

typedef struct lserref_base lserref_base_t;
typedef struct lserref lserref_t;

typedef enum {
  LSERRTYPE_BIND_ENTRY,
  LSERRTYPE_LAMBDA,
} lserrtype_t;

#include "expr/elambda.h"
#include "misc/bind.h"
#include "pat/pref.h"

const lserref_base_t *lserref_base_new_bind_entry(const lsbind_entry_t *ent);
const lserref_base_t *lserref_base_new_lambda(lselambda_t *elambda);
lselambda_t *lserref_base_get_lambda(const lserref_base_t *erref);
const lsbind_entry_t *lserref_base_get_bind_entry(const lserref_base_t *erref);

const lserref_t *lserref_new(const lserref_base_t *erref, const lspref_t *pref);
const lspref_t *lserref_get_pref(const lserref_t *erref);
lserrtype_t lserref_get_type(const lserref_t *erref);
const lsbind_entry_t *lserref_get_bind_entry(const lserref_t *erref);
lselambda_t *lserref_get_lambda(const lserref_t *erref);
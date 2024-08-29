#include "expr/erref.h"
#include "common/malloc.h"
#include "expr/elambda.h"
#include "misc/bind.h"
#include <assert.h>

struct lserref_base {
  lserrtype_t lerrw_type;
  union {
    const lsbind_entry_t *lerrw_bind_entry;
    lselambda_t *lerrw_lambda;
  };
};

const lserref_base_t *
lserref_base_new_bind_entry(const lsbind_entry_t *bentry) {
  assert(bentry != NULL);
  lserref_base_t *const erref_base = lsmalloc(sizeof(lserref_base_t));
  erref_base->lerrw_type = LSERRTYPE_BIND_ENTRY;
  erref_base->lerrw_bind_entry = bentry;
  return erref_base;
}

const lserref_base_t *lserref_base_new_lambda(lselambda_t *elambda) {
  assert(elambda != NULL);
  lserref_base_t *const erref_base = lsmalloc(sizeof(lserref_base_t));
  erref_base->lerrw_type = LSERRTYPE_LAMBDA;
  erref_base->lerrw_lambda = elambda;
  return erref_base;
}

lselambda_t *lserref_base_get_lambda(const lserref_base_t *erref_base) {
  assert(erref_base != NULL);
  assert(erref_base->lerrw_type == LSERRTYPE_LAMBDA);
  return erref_base->lerrw_lambda;
}

const lsbind_entry_t *
lserref_base_get_bind_entry(const lserref_base_t *erref_base) {
  assert(erref_base != NULL);
  assert(erref_base->lerrw_type == LSERRTYPE_BIND_ENTRY);
  return erref_base->lerrw_bind_entry;
}

struct lserref {
  const lserref_base_t *lerr_base;
  const lspref_t *lerr_pref;
};

const lserref_t *lserref_new(const lserref_base_t *erref_base,
                             const lspref_t *pref) {
  assert(erref_base != NULL);
  assert(pref != NULL);
  lserref_t *const erref = lsmalloc(sizeof(lserref_t));
  erref->lerr_base = erref_base;
  erref->lerr_pref = pref;
  return erref;
}

lserrtype_t lserref_get_type(const lserref_t *erref) {
  assert(erref != NULL);
  return erref->lerr_base->lerrw_type;
}

const lspref_t *lserref_get_pref(const lserref_t *erref) {
  assert(erref != NULL);
  return erref->lerr_pref;
}

const lsbind_entry_t *lserref_get_bind_entry(const lserref_t *erref) {
  assert(erref != NULL);
  assert(erref->lerr_base->lerrw_type == LSERRTYPE_BIND_ENTRY);
  return erref->lerr_base->lerrw_bind_entry;
}

lselambda_t *lserref_get_lambda(const lserref_t *erref) {
  assert(erref != NULL);
  assert(erref->lerr_base->lerrw_type == LSERRTYPE_LAMBDA);
  return erref->lerr_base->lerrw_lambda;
}
#pragma once

typedef struct lseenv lseenv_t;
typedef struct lseref_target lseref_target_t;
typedef struct lseref_target_origin lseref_target_origin_t;
typedef enum lseref_target_type {
  LSERTYPE_BIND,
  LSERTYPE_LAMBDA,
  LSERTYPE_EXPR
} lseref_target_type_t;

#include "common/str.h"
#include "thunk/thunk.h"
#include "thunk/tpat.h"

lseenv_t *lseenv_new(const lseenv_t *parent);

lseref_target_t *lseenv_get(const lseenv_t *eenv, const lsstr_t *name);
lseref_target_type_t lseref_target_get_type(const lseref_target_t *target);

void lseenv_put(lseenv_t *eenv, const lsstr_t *name,
                const lseref_target_t *target);

void lseenv_incr_nwarnings(lseenv_t *eenv);
void lseenv_incr_nerrors(lseenv_t *eenv);
void lseenv_incr_nfatales(lseenv_t *eenv);

const lseref_target_t *lseref_target_new(const lseref_target_origin_t *origin,
                                         lstpat_t *tpat);

lseref_target_origin_t *lseref_target_origin_new_bind(lstbind_t *tbind);
lstbind_t *lseref_target_origin_get_bind(const lseref_target_origin_t *origin);
lstbind_t *lseref_target_get_bind(const lseref_target_t *target);

lseref_target_origin_t *
lseref_target_origin_set_rhs(const lseref_target_origin_t *origin,
                             lsthunk_t *rhs);

lseref_target_origin_t *lseref_target_origin_new_lambda(lsthunk_t *thunk);
lsthunk_t *
lseref_target_origin_get_lambda(const lseref_target_origin_t *origin);
lsthunk_t *lseref_target_get_lambda(const lseref_target_t *target);

lseref_target_origin_t *lseref_target_orgin_new_thunk(const lsthunk_t *thunk);
lsthunk_t *lseref_target_origin_get_thunk(const lseref_target_origin_t *origin);
lsthunk_t *lseref_target_get_thunk(const lseref_target_t *target);
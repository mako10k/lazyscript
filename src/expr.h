#pragma once

#include <stdio.h>

typedef struct lsexpr lsexpr_t;

#include "ealge.h"
#include "eappl.h"
#include "eclosure.h"
#include "eenv.h"
#include "elambda.h"
#include "eref.h"
#include "int.h"
#include "str.h"
#include "tenv.h"
#include "thunk.h"

typedef enum {
  LSETYPE_ALGE,
  LSETYPE_APPL,
  LSETYPE_LAMBDA,
  LSETYPE_REF,
  LSETYPE_INT,
  LSETYPE_STR,
  LSETYPE_CLOSURE
} lsetype_t;

lsexpr_t *lsexpr_alge(lsealge_t *ealge);
lsexpr_t *lsexpr_appl(lseappl_t *eappl);
lsexpr_t *lsexpr_ref(lseref_t *eref);
lsexpr_t *lsexpr_int(const lsint_t *eint);
lsexpr_t *lsexpr_str(const lsstr_t *str);
lsexpr_t *lsexpr_lambda(lselambda_t *elambda);
lsexpr_t *lsexpr_closure(lseclosure_t *eclosure);
lsetype_t lsexpr_type(const lsexpr_t *expr);
lsealge_t *lsexpr_get_alge(const lsexpr_t *expr);
lseappl_t *lsexpr_get_appl(const lsexpr_t *expr);
lseref_t *lsexpr_get_ref(const lsexpr_t *expr);
const lsint_t *lsexpr_get_int(const lsexpr_t *expr);
const lsstr_t *lsexpr_get_str(const lsexpr_t *expr);
lselambda_t *lsexpr_get_lambda(const lsexpr_t *expr);
lseclosure_t *lsexpr_get_closure(const lsexpr_t *expr);
void lsexpr_print(FILE *fp, int prec, int indent, const lsexpr_t *expr);
int lsexpr_prepare(lsexpr_t *expr, lseenv_t *env);
lsthunk_t *lsexpr_thunk(lstenv_t *tenv, const lsexpr_t *expr);

#pragma once

#include <stdio.h>

typedef struct lsexpr lsexpr_t;

#include "common/int.h"
#include "common/str.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/echoice.h"
#include "expr/eclosure.h"
#include "expr/eenv.h"
#include "expr/elambda.h"
#include "expr/eref.h"

typedef enum {
  LSETYPE_ALGE,
  LSETYPE_APPL,
  LSETYPE_LAMBDA,
  LSETYPE_REF,
  LSETYPE_INT,
  LSETYPE_STR,
  LSETYPE_CLOSURE,
  LSETYPE_CHOICE,
} lsetype_t;

lsexpr_t *lsexpr_new_alge(lsealge_t *ealge);
lsexpr_t *lsexpr_new_appl(lseappl_t *eappl);
lsexpr_t *lsexpr_new_ref(lseref_t *eref);
lsexpr_t *lsexpr_new_int(const lsint_t *eint);
lsexpr_t *lsexpr_new_str(const lsstr_t *str);
lsexpr_t *lsexpr_new_lambda(lselambda_t *elambda);
lsexpr_t *lsexpr_new_closure(lseclosure_t *eclosure);
lsexpr_t *lsexpr_new_choice(lsechoice_t *echoice);
lsetype_t lsexpr_get_type(const lsexpr_t *expr);
lsealge_t *lsexpr_get_alge(const lsexpr_t *expr);
lseappl_t *lsexpr_get_appl(const lsexpr_t *expr);
lseref_t *lsexpr_get_ref(const lsexpr_t *expr);
const lsint_t *lsexpr_get_int(const lsexpr_t *expr);
const lsstr_t *lsexpr_get_str(const lsexpr_t *expr);
lselambda_t *lsexpr_get_lambda(const lsexpr_t *expr);
lseclosure_t *lsexpr_get_closure(const lsexpr_t *expr);
lsechoice_t *lsexpr_get_choice(const lsexpr_t *expr);
void lsexpr_print(FILE *fp, lsprec_t prec, int indent, const lsexpr_t *expr);
lspres_t lsexpr_prepare(lsexpr_t *expr, lseenv_t *env);
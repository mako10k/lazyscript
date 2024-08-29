#pragma once

#include <stdio.h>

typedef struct lsexpr lsexpr_t;

#include "common/int.h"
#include "common/str.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/echoice.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"

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

const lsexpr_t *lsexpr_new_alge(const lsealge_t *ealge);
const lsexpr_t *lsexpr_new_appl(const lseappl_t *eappl);
const lsexpr_t *lsexpr_new_ref(const lsref_t *ref);
const lsexpr_t *lsexpr_new_int(const lsint_t *eint);
const lsexpr_t *lsexpr_new_str(const lsstr_t *str);
const lsexpr_t *lsexpr_new_lambda(const lselambda_t *elambda);
const lsexpr_t *lsexpr_new_closure(const lseclosure_t *eclosure);
const lsexpr_t *lsexpr_new_choice(const lsechoice_t *echoice);
lsetype_t lsexpr_get_type(const lsexpr_t *expr);
const lsealge_t *lsexpr_get_alge(const lsexpr_t *expr);
const lseappl_t *lsexpr_get_appl(const lsexpr_t *expr);
const lsref_t *lsexpr_get_ref(const lsexpr_t *expr);
const lsint_t *lsexpr_get_int(const lsexpr_t *expr);
const lsstr_t *lsexpr_get_str(const lsexpr_t *expr);
const lselambda_t *lsexpr_get_lambda(const lsexpr_t *expr);
const lseclosure_t *lsexpr_get_closure(const lsexpr_t *expr);
const lsechoice_t *lsexpr_get_choice(const lsexpr_t *expr);
void lsexpr_print(FILE *fp, lsprec_t prec, int indent, const lsexpr_t *expr);
#pragma once

#include <stdio.h>

typedef struct lsexpr lsexpr_t;

#include "appl.h"
#include "ealge.h"
#include "eref.h"
#include "int.h"
#include "lambda.h"
#include "str.h"

typedef enum {
  LSETYPE_VOID = 0,
  LSETYPE_ALGE,
  LSETYPE_APPL,
  LSETYPE_LAMBDA,
  LSETYPE_REF,
  LSETYPE_INT,
  LSETYPE_STR
} lsetype_t;

lsexpr_t *lsexpr_alge(lsealge_t *ealge);
lsexpr_t *lsexpr_appl(lsappl_t *eappl);
lsexpr_t *lsexpr_ref(lseref_t *eref);
lsexpr_t *lsexpr_int(const lsint_t *eint);
lsexpr_t *lsexpr_str(const lsstr_t *str);
lsexpr_t *lsexpr_lambda(lslambda_t *lambda);

void lsexpr_print(FILE *fp, int prec, const lsexpr_t *expr);

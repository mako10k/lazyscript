#pragma once

typedef struct lsthunk lsthunk_t;

#include "int.h"
#include "str.h"
#include "talge.h"
#include "tappl.h"
#include "tclosure.h"
#include "tenv.h"
#include "tlambda.h"
#include "tref.h"

typedef enum lsttype {
  LSTTYPE_ALGE,
  LSTTYPE_APPL,
  LSTTYPE_INT,
  LSTTYPE_STR,
  LSTTYPE_CLOSURE,
  LSTTYPE_LAMBDA,
  LSTTYPE_REF
} lsttype_t;

lsthunk_t *lsthunk_int(const lsint_t *intval);
lsthunk_t *lsthunk_str(const lsstr_t *strval);
lsthunk_t *lsthunk_appl(lstappl_t *tappl);
lsthunk_t *lsthunk_alge(lstalge_t *talge);
lsthunk_t *lsthunk_lambda(lstlambda_t *tlambda);
lsthunk_t *lsthunk_closure(lstclosure_t *tclosure);
lsthunk_t *lsthunk_ref(lstref_t *tref);
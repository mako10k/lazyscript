#pragma once

typedef struct lsexpr lsexpr_t;

#include "appl.h"
#include "ealge.h"
#include "eref.h"
#include "int.h"

typedef enum {
  LSETYPE_VOID = 0,
  LSETYPE_ALGE,
  LSETYPE_APPL,
  LSETYPE_REF,
  LSETYPE_INT,
} lsetype_t;

lsexpr_t *lsexpr_alge(lsealge_t *ealge);
lsexpr_t *lsexpr_appl(lsappl_t *eappl);
lsexpr_t *lsexpr_ref(lseref_t *eref);
lsexpr_t *lsexpr_int(const lsint_t *eint);

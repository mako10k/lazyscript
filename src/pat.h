#pragma once

typedef struct lspat lspat_t;

#include "pas.h"
#include "eenv.h"
#include "int.h"
#include "palge.h"
#include "pref.h"

lspat_t *lspat_alge(lspalge_t *alge);
lspat_t *lspat_as(lspas_t *as);
lspat_t *lspat_int(const lsint_t *intval);
lspat_t *lspat_str(const lsstr_t *strval);
lspat_t *lspat_ref(lspref_t *pref);
void lspat_print(FILE *fp, int prec, int indent, const lspat_t *pat);
int lspat_prepare(lspat_t *pat, lseenv_t *env, lserref_wrapper_t *erref);

typedef enum lsptype {
  LSPTYPE_ALGE,
  LSPTYPE_AS,
  LSPTYPE_INT,
  LSPTYPE_STR,
  LSPTYPE_REF
} lsptype_t;
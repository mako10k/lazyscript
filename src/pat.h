#pragma once

typedef struct lspat lspat_t;

typedef enum lsptype {
  LSPTYPE_ALGE,
  LSPTYPE_AS,
  LSPTYPE_INT,
  LSPTYPE_STR,
  LSPTYPE_REF
} lsptype_t;

#include "eenv.h"
#include "int.h"
#include "palge.h"
#include "pas.h"
#include "pref.h"
#include "tenv.h"
#include "thunk.h"

lspat_t *lspat_new_alge(lspalge_t *alge);
lspat_t *lspat_new_as(lspas_t *as);
lspat_t *lspat_new_int(const lsint_t *intval);
lspat_t *lspat_new_str(const lsstr_t *strval);
lspat_t *lspat_new_ref(lspref_t *pref);
void lspat_print(FILE *fp, lsprec_t prec, int indent, const lspat_t *pat);
lspres_t lspat_prepare(lspat_t *pat, lseenv_t *env, lserref_wrapper_t *erref);
lsmres_t lspat_match(lstenv_t *tenv, const lspat_t *pat, lsthunk_t *think);

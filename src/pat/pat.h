#pragma once

typedef struct lspat lspat_t;

typedef enum lsptype {
  LSPTYPE_ALGE,
  LSPTYPE_AS,
  LSPTYPE_INT,
  LSPTYPE_STR,
  LSPTYPE_REF
} lsptype_t;

#include "common/int.h"
#include "expr/eenv.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pref.h"

lspat_t *lspat_new_alge(lspalge_t *alge);
lspat_t *lspat_new_as(lspas_t *as);
lspat_t *lspat_new_int(const lsint_t *intval);
lspat_t *lspat_new_str(const lsstr_t *strval);
lspat_t *lspat_new_ref(lspref_t *pref);
lsptype_t lspat_get_type(const lspat_t *pat);
lspalge_t *lspat_get_alge(const lspat_t *pat);
lspas_t *lspat_get_as(const lspat_t *pat);
const lsint_t *lspat_get_int(const lspat_t *pat);
const lsstr_t *lspat_get_str(const lspat_t *pat);
lspref_t *lspat_get_ref(const lspat_t *pat);
void lspat_print(FILE *fp, lsprec_t prec, int indent, const lspat_t *pat);
lspres_t lspat_prepare(lspat_t *pat, lseenv_t *env,
                       const lserref_base_t *erref);

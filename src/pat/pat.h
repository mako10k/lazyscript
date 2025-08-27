#pragma once

typedef struct lspat lspat_t;

typedef enum lsptype {
  LSPTYPE_ALGE,
  LSPTYPE_AS,
  LSPTYPE_INT,
  LSPTYPE_STR,
  LSPTYPE_REF,
  LSPTYPE_WILDCARD,
  LSPTYPE_OR
} lsptype_t;

#include "common/int.h"
#include "common/ref.h"
#include "pat/palge.h"
#include "pat/pas.h"

const lspat_t*   lspat_new_alge(const lspalge_t* palge);
const lspat_t*   lspat_new_as(const lspas_t* pas);
const lspat_t*   lspat_new_int(const lsint_t* val);
const lspat_t*   lspat_new_str(const lsstr_t* val);
const lspat_t*   lspat_new_ref(const lsref_t* ref);
const lspat_t*   lspat_new_wild(void);
const lspat_t*   lspat_new_or(const lspat_t* left, const lspat_t* right);
lsptype_t        lspat_get_type(const lspat_t* pat);
const lspalge_t* lspat_get_alge(const lspat_t* pat);
const lspas_t*   lspat_get_as(const lspat_t* pat);
const lsint_t*   lspat_get_int(const lspat_t* pat);
const lsstr_t*   lspat_get_str(const lspat_t* pat);
const lsref_t*   lspat_get_ref(const lspat_t* pat);
const lspat_t*   lspat_get_or_left(const lspat_t* pat);
const lspat_t*   lspat_get_or_right(const lspat_t* pat);
void             lspat_print(FILE* fp, lsprec_t prec, int indent, const lspat_t* pat);
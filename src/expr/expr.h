#pragma once

#include <stdio.h>

// Forward-declare location type to avoid include cycles; implementation includes common/loc.h
typedef struct lsloc lsloc_t;

/** Expression. */
typedef struct lsexpr lsexpr_t;

#include "common/int.h"
#include "common/str.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/echoice.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"
#include "expr/enslit.h"

/** Expression type. */
typedef enum {
  LSETYPE_ALGE,
  LSETYPE_APPL,
  LSETYPE_LAMBDA,
  LSETYPE_REF,
  LSETYPE_INT,
  LSETYPE_STR,
  LSETYPE_CLOSURE,
  LSETYPE_CHOICE,
  LSETYPE_NSLIT,
  // First-class dot symbol literal (e.g., .name) — distinct from algebraic constructors
  LSETYPE_SYMBOL,
} lsetype_t;

#define lsapi_expr_new lsapi_nn1 lsapi_wur

lsapi_expr_new const lsexpr_t* lsexpr_new_alge(const lsealge_t* ealge);
lsapi_expr_new const lsexpr_t* lsexpr_new_appl(const lseappl_t* eappl);
lsapi_expr_new const lsexpr_t* lsexpr_new_ref(const lsref_t* ref);
lsapi_expr_new const lsexpr_t* lsexpr_new_int(const lsint_t* eint);
lsapi_expr_new const lsexpr_t* lsexpr_new_str(const lsstr_t* str);
lsapi_expr_new const lsexpr_t* lsexpr_new_lambda(const lselambda_t* elambda);
lsapi_expr_new const lsexpr_t* lsexpr_new_closure(const lseclosure_t* eclosure);
lsapi_expr_new const lsexpr_t* lsexpr_new_choice(const lsechoice_t* echoice);
lsapi_expr_new const lsexpr_t* lsexpr_new_nslit(const lsenslit_t* ens);
// Create a new expression for a dot-prefixed symbol (e.g., .name)
lsapi_expr_new const lsexpr_t* lsexpr_new_symbol(const lsstr_t* sym);
lsapi_get lsetype_t            lsexpr_get_type(const lsexpr_t* expr);
lsapi_get const lsealge_t*     lsexpr_get_alge(const lsexpr_t* expr);
lsapi_get const lseappl_t*     lsexpr_get_appl(const lsexpr_t* expr);
lsapi_get const lsref_t*       lsexpr_get_ref(const lsexpr_t* expr);
lsapi_get const lsint_t*       lsexpr_get_int(const lsexpr_t* expr);
lsapi_get const lsstr_t*       lsexpr_get_str(const lsexpr_t* expr);
lsapi_get const lselambda_t*   lsexpr_get_lambda(const lsexpr_t* expr);
lsapi_get const lseclosure_t*  lsexpr_get_closure(const lsexpr_t* expr);
lsapi_get const lsechoice_t*   lsexpr_get_choice(const lsexpr_t* expr);
lsapi_get const lsenslit_t*    lsexpr_get_nslit(const lsexpr_t* expr);
// Accessor for symbol literal
lsapi_get const lsstr_t*       lsexpr_get_symbol(const lsexpr_t* expr);
lsapi_print void lsexpr_print(FILE* fp, lsprec_t prec, int indent, const lsexpr_t* expr);

// Lightweight query helpers for printers
typedef enum lsexpr_type_query {
  LSEQ_ALGE = LSETYPE_ALGE,
  LSEQ_APPL = LSETYPE_APPL,
  LSEQ_REF = LSETYPE_REF,
  LSEQ_INT = LSETYPE_INT,
  LSEQ_STR = LSETYPE_STR,
  LSEQ_LAMBDA = LSETYPE_LAMBDA,
  LSEQ_CLOSURE = LSETYPE_CLOSURE,
  LSEQ_CHOICE = LSETYPE_CHOICE,
  LSEQ_NSLIT = LSETYPE_NSLIT,
  LSEQ_SYMBOL = LSETYPE_SYMBOL
} lsexpr_type_query_t;

lsapi_get lsexpr_type_query_t lsexpr_typeof(const lsexpr_t* expr);

// Location helpers
lsapi_get lsloc_t              lsexpr_get_loc(const lsexpr_t* expr);
// Attach a source location to an expression and return it (same pointer)
lsapi_nn1 lsapi_wur const lsexpr_t* lsexpr_with_loc(const lsexpr_t* expr, lsloc_t loc);
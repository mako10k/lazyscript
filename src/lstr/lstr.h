#pragma once

#include <stdio.h>

/* Forward decls */
typedef struct lstr_prog lstr_prog_t;
typedef struct lstr_expr lstr_expr_t;
typedef struct lstr_val  lstr_val_t;
typedef struct lstr_pat  lstr_pat_t;

/* Minimal kinds (v1) */
typedef enum {
  LSTR_VAL_INT,
  LSTR_VAL_STR,
  LSTR_VAL_REF,
  LSTR_VAL_CONSTR,
  LSTR_VAL_LAM,
  LSTR_VAL_BOTTOM,
  LSTR_VAL_CHOICE,
} lstr_val_kind_t;

typedef enum {
  LSTR_PAT_WILDCARD,
  LSTR_PAT_VAR,
  LSTR_PAT_CONSTR,
  LSTR_PAT_INT,
  LSTR_PAT_STR,
  LSTR_PAT_AS,
  LSTR_PAT_OR,
  LSTR_PAT_CARET,
} lstr_pat_kind_t;

typedef enum {
  LSTR_EXP_VAL,
  LSTR_EXP_LET,
  LSTR_EXP_APP,
  LSTR_EXP_MATCH,
  LSTR_EXP_EFFAPP,
  LSTR_EXP_TOKEN,
} lstr_exp_kind_t;

/* API skeleton */
const lstr_prog_t* lstr_read_text(FILE* in, FILE* err);
void               lstr_print(FILE* out, int indent, const lstr_prog_t* p);
int                lstr_validate(const lstr_prog_t* p);

// Convenience: build LSTR from an LSTI file path (minimal v1)
const lstr_prog_t* lstr_from_lsti_path(const char* path);

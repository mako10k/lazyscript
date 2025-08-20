#pragma once

typedef struct lscir_prog lscir_prog_t;

#include <stdio.h>
#include "misc/prog.h"

/*
 * Minimal Core IR data model (ANF 風・暫定)
 */
typedef enum {
  LCIR_VAL_INT,
  LCIR_VAL_STR,
  LCIR_VAL_CONSTR,
  LCIR_VAL_VAR,
  LCIR_VAL_LAM,
} lscir_val_kind_t;

typedef enum {
  LCIR_EXP_VAL,    // 値（末尾式）
  LCIR_EXP_LET,    // let x = e1 in e2
  LCIR_EXP_APP,    // f(vs...)
  LCIR_EXP_IF,     // if v then e1 else e2
  LCIR_EXP_EFFAPP, // eff(vs..., token)
  LCIR_EXP_TOKEN,  // 効果トークン生成
} lscir_exp_kind_t;

typedef struct lscir_value lscir_value_t;
typedef struct lscir_expr  lscir_expr_t;

struct lscir_value {
  lscir_val_kind_t kind;
  union {
    long long   ival;
    const char* sval;
    struct {
      const char*                 name;
      const lscir_value_t* const* args;
      int                         argc;
    } constr;
    const char* var; // 変数名（デバッグ用）
    struct {
      const char*         param; // 単一引数（暫定）
      const lscir_expr_t* body;
    } lam;
  };
};

struct lscir_expr {
  lscir_exp_kind_t kind;
  union {
    const lscir_value_t* v; // LCIR_EXP_VAL
    struct {                // LCIR_EXP_LET
      const char*         var;
      const lscir_expr_t* bind;
      const lscir_expr_t* body;
    } let1;
    struct { // LCIR_EXP_APP
      const lscir_value_t*        func;
      const lscir_value_t* const* args;
      int                         argc;
    } app;
    struct { // LCIR_EXP_IF
      const lscir_value_t* cond;
      const lscir_expr_t*  then_e;
      const lscir_expr_t*  else_e;
    } ife;
    struct { // LCIR_EXP_EFFAPP
      const lscir_value_t*        func;
      const lscir_value_t*        token;
      const lscir_value_t* const* args;
      int                         argc;
    } effapp;
  };
};

/* Program root */
struct lscir_prog;

/* Lower a parsed program to Core IR (暫定実装: AST ラッパーのみ) */
const lscir_prog_t* lscir_lower_prog(const lsprog_t* prog);

/* Print a Core IR program */
void lscir_print(FILE* fp, int indent, const lscir_prog_t* cir);

/* Validate effect/token discipline; returns error count (0 = OK). */
int lscir_validate_effects(FILE* errfp, const lscir_prog_t* cir);

/* Evaluate a Core IR program (minimal, for smoke tests). Returns 0 on success.
 * Writes any effect output (e.g., println) to stdout, and prints the final value
 * in a simple form compatible with existing tests (e.g., () for unit).
 */
int lscir_eval(FILE* outfp, const lscir_prog_t* cir);

/* Minimal typecheck for Core IR: prints a one-line result to outfp.
 * Returns 0 on success, non-zero on type error.
 */
int lscir_typecheck(FILE* outfp, const lscir_prog_t* cir);

/* Configure Kind check behavior for lscir_typecheck (process-global, minimal).
 * warn_enabled: non-zero to emit warnings to stderr when IO (effects) are detected.
 * error_enabled: non-zero to treat IO detection as an error (non-zero exit),
 *                printing error details to stderr. When enabled, warning is suppressed.
 */
void lscir_typecheck_set_kind_warn(int warn_enabled);
void lscir_typecheck_set_kind_error(int error_enabled);

#pragma once

#include <stddef.h>
#include "lstr.h"
#include "common/str.h"

typedef struct lstr_pat  lstr_pat_t;
typedef struct lstr_val  lstr_val_t;
typedef struct lstr_expr lstr_expr_t;

struct lstr_pat {
  lstr_pat_kind_t kind;
  union {
    const lsstr_t* var_name; // VAR
    struct {                 // CONSTR
      const lsstr_t*    name;
      size_t            argc;
      struct lstr_pat** args;
    } constr;
    long long      i;           // INT
    const lsstr_t* s;           // STR
    struct {                    // AS
      struct lstr_pat* ref_pat; // usually VAR/REF
      struct lstr_pat* inner;
    } as_pat;
    struct { // OR
      struct lstr_pat* left;
      struct lstr_pat* right;
    } or_pat;
    struct lstr_pat* caret_inner; // CARET
  } as;
};

struct lstr_val {
  lstr_val_kind_t kind;
  union {
    long long      i;        // INT
    const lsstr_t* s;        // STR
    const lsstr_t* ref_name; // REF (and SYMBOL)
    struct {                 // CONSTR
      const lsstr_t* name;
      size_t         argc;
      lstr_val_t**   args; // children as values
    } constr;
    struct { // LAM
      lstr_pat_t*  param;
      lstr_expr_t* body;
    } lam;
    struct { // BOTTOM
      const char*  msg;
      size_t       relc;
      lstr_val_t** related;
    } bottom;
    struct { // CHOICE (minimal)
      int          kind;
      lstr_expr_t* left;
      lstr_expr_t* right;
    } choice;
  } as;
};

struct lstr_expr {
  lstr_exp_kind_t kind;
  union {
    lstr_val_t* v; // EXP_VAL
    struct {
      lstr_expr_t*  func;
      size_t        argc;
      lstr_expr_t** args;
    } app; // EXP_APP
  } as;
};

struct lstr_prog {
  size_t        rootc;
  lstr_expr_t** roots;
};

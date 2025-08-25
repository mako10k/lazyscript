#pragma once

typedef struct lsprog lsprog_t;
typedef struct lsscan lsscan_t;

#include "expr/expr.h"
#include "common/loc.h"
#include "common/str.h"
#include "common/array.h"

// Lightweight comment record
typedef struct lscomment {
	lsloc_t        lc_loc;
	const lsstr_t* lc_text; // raw comment text as it appeared (e.g., "#..." or "{- ... -}")
} lscomment_t;

// Create program with expression and optional comments list (array of lscomment_t*)
const lsprog_t* lsprog_new(const lsexpr_t* expr, const lsarray_t* comments);
void            lsprog_print(FILE* fp, int prec, int indent, const lsprog_t* prog);
const lsexpr_t* lsprog_get_expr(const lsprog_t* prog);
const lsarray_t* lsprog_get_comments(const lsprog_t* prog);

void            yyerror(lsloc_t* loc, lsscan_t* scanner, const char* s);

lsscan_t*       lsscan_new(const char* filename);
const lsprog_t* lsscan_get_prog(const lsscan_t* scanner);
void            lsscan_set_prog(lsscan_t* scanner, const lsprog_t* prog);
const char*     lsscan_get_filename(const lsscan_t* scanner);
// Sugar namespace for ~~sym desugaring (default: "prelude")
void        lsscan_set_sugar_ns(lsscan_t* scanner, const char* ns);
const char* lsscan_get_sugar_ns(const lsscan_t* scanner);

// Comments collection API (used by lexer)
void            lsscan_add_comment(lsscan_t* scanner, lsloc_t loc, const lsstr_t* text);
// Transfer ownership of accumulated comments to caller, resetting internal storage
const lsarray_t* lsscan_take_comments(lsscan_t* scanner);

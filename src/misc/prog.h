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

// --- Scanner whitespace/tight-adjacency helpers ---
// Mark that whitespace was seen since the last token.
void lsscan_note_ws(lsscan_t* scanner);
// Consume and return current tight-adjacency flag (1 when no whitespace since previous token and at least one token was produced).
int  lsscan_consume_tight(lsscan_t* scanner);

// Comment weaving API (used by formatter):
// Enable interleaving of source comments during printing. When active,
// printers may call lsfmt_flush_comments_up_to to emit comments whose
// original line is <= the given line.
void lsfmt_set_comment_stream(const lsarray_t* comments);
void lsfmt_flush_comments_up_to(FILE* fp, int line, int indent);
void lsfmt_clear_comment_stream(void);
int  lsfmt_is_active(void);

// Peek the next comment without consuming; returns NULL if none.
const lscomment_t* lsfmt_peek_next_comment(void);
// Consume the next comment (advance index) if any.
void lsfmt_consume_next_comment(void);
// Hold EOL comments on a specific source line so nested printers don't emit them.
void lsfmt_set_hold_line(int line);
void lsfmt_clear_hold_line(void);

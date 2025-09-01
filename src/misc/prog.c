#include "misc/prog.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>
#include <string.h>

struct lsprog {
  const lsexpr_t*  lp_expr;
  const lsarray_t* lp_comments; // array of lscomment_t*
};

struct lsscan {
  const lsprog_t*  ls_prog;
  const char*      ls_filename;
  const char*      ls_sugar_ns;
  const lsarray_t* ls_comments;         // array of lscomment_t*
  int              ls_seen_token;       // have we emitted any token yet?
  int              ls_tight_since_last; // no whitespace since last token
};

const lsprog_t* lsprog_new(const lsexpr_t* expr, const lsarray_t* comments) {
  lsprog_t* prog    = lsmalloc(sizeof(lsprog_t));
  prog->lp_expr     = expr;
  prog->lp_comments = comments;
  return prog;
}

void lsprog_print(FILE* fp, int prec, int indent, const lsprog_t* prog) {
  // If comment weaving is active, flush comments up to the first line of program expr
  if (lsfmt_is_active()) {
    int first_line = 0;
    if (prog && prog->lp_expr)
      first_line = lsexpr_get_loc(prog->lp_expr).first_line;
    lsfmt_flush_comments_up_to(fp, first_line ? first_line : 1, indent);
  }
  lsexpr_print(fp, prec, indent, prog->lp_expr);
  lsprintf(fp, 0, ";\n");
}

const lsexpr_t*  lsprog_get_expr(const lsprog_t* prog) { return prog->lp_expr; }
const lsarray_t* lsprog_get_comments(const lsprog_t* prog) { return prog->lp_comments; }

void             yyerror(lsloc_t* loc, lsscan_t* scanner, const char* s) {
              (void)scanner;
              lsloc_print(stderr, *loc);
              lsprintf(stderr, 0, "%s\n", s);
}

lsscan_t* lsscan_new(const char* filename) {
  lsscan_t* scanner            = lsmalloc(sizeof(lsscan_t));
  scanner->ls_prog             = NULL;
  scanner->ls_filename         = filename;
  scanner->ls_sugar_ns         = "prelude";
  scanner->ls_comments         = NULL;
  scanner->ls_seen_token       = 0;
  scanner->ls_tight_since_last = 0;
  return scanner;
}

const lsprog_t* lsscan_get_prog(const lsscan_t* scanner) { return scanner->ls_prog; }

void        lsscan_set_prog(lsscan_t* scanner, const lsprog_t* prog) { scanner->ls_prog = prog; }

const char* lsscan_get_filename(const lsscan_t* scanner) { return scanner->ls_filename; }

void        lsscan_set_sugar_ns(lsscan_t* scanner, const char* ns) {
         scanner->ls_sugar_ns = (ns && ns[0]) ? ns : "prelude";
}

const char* lsscan_get_sugar_ns(const lsscan_t* scanner) {
  return scanner->ls_sugar_ns ? scanner->ls_sugar_ns : "prelude";
}

void lsscan_add_comment(lsscan_t* scanner, lsloc_t loc, const lsstr_t* text) {
  if (!scanner)
    return;
  lscomment_t* c = lsmalloc(sizeof(lscomment_t));
  c->lc_loc      = loc;
  c->lc_text     = text;
  if (!scanner->ls_comments) {
    scanner->ls_comments = lsarray_new(1, c);
  } else {
    scanner->ls_comments = lsarray_push((lsarray_t*)scanner->ls_comments, (void*)c);
  }
}

const lsarray_t* lsscan_take_comments(lsscan_t* scanner) {
  const lsarray_t* out = scanner ? scanner->ls_comments : NULL;
  if (scanner)
    scanner->ls_comments = NULL;
  return out;
}

void lsscan_note_ws(lsscan_t* scanner) {
  if (!scanner)
    return;
  scanner->ls_tight_since_last = 0;
}

int lsscan_consume_tight(lsscan_t* scanner) {
  if (!scanner)
    return 0;
  int tight = (scanner->ls_seen_token && scanner->ls_tight_since_last) ? 1 : 0;
  // After consuming, mark that next token will consider adjacency anew
  scanner->ls_seen_token       = 1;
  scanner->ls_tight_since_last = 1; // assume tight until whitespace noted
  return tight;
}

// --- Comment weaving runtime (simplistic, global state for formatter run) ---
static const lsarray_t* g_fmt_comments  = NULL; // array of lscomment_t*
static lssize_t         g_fmt_index     = 0;
static int              g_fmt_hold_line = 0;
// Formatter activation flag (decoupled from presence of comments). When true,
// printers may apply formatter-specific behavior even if there are no comments.
static int g_fmt_active = 0;

void       lsfmt_set_comment_stream(const lsarray_t* comments) {
        g_fmt_comments  = comments;
        g_fmt_index     = 0;
        g_fmt_hold_line = 0;
        g_fmt_active    = 1;
}

void lsfmt_clear_comment_stream(void) {
  g_fmt_comments  = NULL;
  g_fmt_index     = 0;
  g_fmt_hold_line = 0;
  g_fmt_active    = 0;
}

int  lsfmt_is_active(void) { return g_fmt_active; }

void lsfmt_flush_comments_up_to(FILE* fp, int line, int indent) {
  if (!g_fmt_active)
    return;
  if (!g_fmt_comments)
    return; // active but no comments to flush
  const void* const* pv = lsarray_get(g_fmt_comments);
  lssize_t           n  = lsarray_get_size(g_fmt_comments);
  while (g_fmt_index < n) {
    const lscomment_t* c = (const lscomment_t*)pv[g_fmt_index];
    if (!c || c->lc_loc.first_line > line)
      break;
    if (g_fmt_hold_line && c->lc_loc.first_line == g_fmt_hold_line)
      break; // keep EOL for caller
    if (c->lc_text) {
      const char* s = lsstr_get_buf(c->lc_text);
      if (s && s[0]) {
        // print as-is with given indent (line begins with '#' or not)
        lsprintf(fp, indent, "%s\n", s);
      }
    }
    g_fmt_index++;
  }
}

const lscomment_t* lsfmt_peek_next_comment(void) {
  if (!g_fmt_comments)
    return NULL;
  const void* const* pv = lsarray_get(g_fmt_comments);
  lssize_t           n  = lsarray_get_size(g_fmt_comments);
  if (g_fmt_index >= n)
    return NULL;
  return (const lscomment_t*)pv[g_fmt_index];
}

void lsfmt_consume_next_comment(void) {
  if (!g_fmt_comments)
    return;
  lssize_t n = lsarray_get_size(g_fmt_comments);
  if (g_fmt_index < n)
    g_fmt_index++;
}

void lsfmt_set_hold_line(int line) { g_fmt_hold_line = line; }
void lsfmt_clear_hold_line(void) { g_fmt_hold_line = 0; }
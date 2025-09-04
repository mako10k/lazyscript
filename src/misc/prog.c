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
  int              ls_had_error;        // set when yyerror is called
  // include support: stack of filenames (lsstr_t* or raw cstr owned externally)
  const lsarray_t* ls_file_stack;       // array of const char* (push on include)
  const lsarray_t* ls_fp_stack;         // array of FILE* parallel to filenames
  const lsarray_t* ls_inc_sites;        // array of lsloc_t* include sites (one per push)
  int              ls_have_inc_sites;   // seen any include-push during this scan
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

void             lsscan_print_include_chain(FILE* fp, const lsscan_t* scanner) {
  if (!scanner || !scanner->ls_have_inc_sites || !scanner->ls_inc_sites)
    return;
  lssize_t               n  = lsarray_get_size(scanner->ls_inc_sites);
  const void* const*     pv = lsarray_get(scanner->ls_inc_sites);
  if (n <= 0 || pv == NULL)
    return; // nothing to print or corrupted/empty array
  for (lssize_t i = n; i > 0; --i) {
    const lsloc_t* site = (const lsloc_t*)pv[i - 1];
    if (!site)
      continue;
    lsprintf(fp, 0, "In file included from ");
    lsloc_print(fp, *site);
    lsprintf(fp, 0, "\n");
  }
}

void             lsscan_mark_error(lsscan_t* scanner) { if (scanner) scanner->ls_had_error = 1; }
int              lsscan_has_error(const lsscan_t* scanner) { return scanner ? scanner->ls_had_error : 0; }

void             yyerror(lsloc_t* loc, lsscan_t* scanner, const char* s) {
  if (scanner) lsscan_mark_error(scanner);
  // Print include chain only if explicitly enabled via env and include was used
  const char* incdiag = getenv("LAZYFMT_PRINT_INCLUDE_CHAIN");
  if (incdiag && incdiag[0] && incdiag[0] != '0') {
    if (scanner && scanner->ls_have_inc_sites && scanner->ls_inc_sites &&
        lsarray_get(scanner->ls_inc_sites) != NULL && lsarray_get_size(scanner->ls_inc_sites) >= 0) {
      lsscan_print_include_chain(stderr, scanner);
    }
  }
  if (loc) lsloc_print(stderr, *loc);
  lsprintf(stderr, 0, "%s\n", s ? s : "error");
}

lsscan_t* lsscan_new(const char* filename) {
  lsscan_t* scanner            = lsmalloc(sizeof(lsscan_t));
  scanner->ls_prog             = NULL;
  scanner->ls_filename         = filename;
  scanner->ls_sugar_ns         = "prelude";
  scanner->ls_comments         = NULL;
  scanner->ls_seen_token       = 0;
  scanner->ls_tight_since_last = 0;
  scanner->ls_had_error        = 0;
  scanner->ls_file_stack       = NULL;
  scanner->ls_fp_stack         = NULL;
  scanner->ls_inc_sites        = NULL;
  scanner->ls_have_inc_sites   = 0;
  return scanner;
}

const lsprog_t* lsscan_get_prog(const lsscan_t* scanner) { return scanner->ls_prog; }

void        lsscan_set_prog(lsscan_t* scanner, const lsprog_t* prog) { scanner->ls_prog = prog; }

const char* lsscan_get_filename(const lsscan_t* scanner) { return scanner->ls_filename; }

void lsscan_push_filename(lsscan_t* scanner, const char* filename) {
  if (!scanner || !filename) return;
  // push current name into stack
  if (!scanner->ls_file_stack) scanner->ls_file_stack = lsarray_new(1, (void*)scanner->ls_filename);
  else scanner->ls_file_stack = lsarray_push((lsarray_t*)scanner->ls_file_stack, (void*)scanner->ls_filename);
  scanner->ls_filename = filename;
}

const char* lsscan_pop_filename(lsscan_t* scanner) {
  if (!scanner) return NULL;
  const char* prev = NULL;
  if (scanner->ls_file_stack) {
    lssize_t n = lsarray_get_size(scanner->ls_file_stack);
    if (n > 0) {
      const void* const* pv = lsarray_get(scanner->ls_file_stack);
      prev = (const char*)pv[n - 1];
      // rebuild stack without the last element
      if (n == 1) scanner->ls_file_stack = NULL;
      else {
        // create a new C-array with first n-1 items and wrap into lsarray via lsarray_newv
        // lsarray API doesn't expose slice; emulate by pushing into a new array
        const lsarray_t* na = NULL;
        for (lssize_t i = 0; i < n - 1; ++i) {
          const void* v = pv[i];
          na = lsarray_push(na, v);
        }
        scanner->ls_file_stack = na;
      }
    }
  }
  if (prev) scanner->ls_filename = prev;
  return prev;
}

int lsscan_in_include_chain(const lsscan_t* scanner, const char* filename) {
  if (!scanner || !filename) return 0;
  if (scanner->ls_filename && strcmp(scanner->ls_filename, filename) == 0) return 1;
  if (!scanner->ls_file_stack) return 0;
  const void* const* pv = lsarray_get(scanner->ls_file_stack);
  lssize_t n = lsarray_get_size(scanner->ls_file_stack);
  for (lssize_t i = 0; i < n; ++i) {
    const char* f = (const char*)pv[i];
    if (f && strcmp(f, filename) == 0) return 1;
  }
  return 0;
}

void lsscan_push_file_fp(lsscan_t* scanner, FILE* fp) {
  if (!scanner) return;
  scanner->ls_fp_stack = lsarray_push(scanner->ls_fp_stack, fp);
}

FILE* lsscan_pop_file_fp(lsscan_t* scanner) {
  if (!scanner || !scanner->ls_fp_stack) return NULL;
  lssize_t n = lsarray_get_size(scanner->ls_fp_stack);
  if (n == 0) return NULL;
  const void* const* pv = lsarray_get(scanner->ls_fp_stack);
  FILE* fp = (FILE*)pv[n - 1];
  const lsarray_t* na = NULL;
  for (lssize_t i = 0; i < n - 1; ++i) na = lsarray_push(na, pv[i]);
  scanner->ls_fp_stack = na;
  return fp;
}

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
// Optional: disable resugaring (do-notation, dot-chain fusion, include re-emit)
// when set to non-empty env LAZYFMT_DISABLE_RESUGAR.
static int g_fmt_disable_resugar = -1; // lazy-init from env

void       lsfmt_set_comment_stream(const lsarray_t* comments) {
        g_fmt_comments  = comments;
        g_fmt_index     = 0;
        g_fmt_hold_line = 0;
        g_fmt_active    = 1;
        if (g_fmt_disable_resugar < 0) {
          const char* v = getenv("LAZYFMT_DISABLE_RESUGAR");
          g_fmt_disable_resugar = (v && v[0] && v[0] != '0') ? 1 : 0;
        }
}

void lsfmt_clear_comment_stream(void) {
  g_fmt_comments  = NULL;
  g_fmt_index     = 0;
  g_fmt_hold_line = 0;
  g_fmt_active    = 0;
}

int  lsfmt_is_active(void) { return g_fmt_active; }

int  lsfmt_is_resugar_disabled(void) {
  if (g_fmt_disable_resugar < 0) {
    const char* v = getenv("LAZYFMT_DISABLE_RESUGAR");
    g_fmt_disable_resugar = (v && v[0] && v[0] != '0') ? 1 : 0;
  }
  return g_fmt_disable_resugar;
}

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

// --- include site stack helpers ---
void lsscan_push_include_site(lsscan_t* scanner, lsloc_t site) {
  if (!scanner) return;
  lsloc_t* p = lsmalloc(sizeof(lsloc_t));
  *p         = site;
  scanner->ls_inc_sites = lsarray_push(scanner->ls_inc_sites, p);
  scanner->ls_have_inc_sites = 1;
}

void lsscan_pop_include_site(lsscan_t* scanner) {
  if (!scanner || !scanner->ls_inc_sites) return;
  lssize_t n = lsarray_get_size(scanner->ls_inc_sites);
  if (n == 0) return;
  const void* const* pv = lsarray_get(scanner->ls_inc_sites);
  lsfree((void*)pv[n - 1]);
  const lsarray_t* na = NULL;
  for (lssize_t i = 0; i < n - 1; ++i) na = lsarray_push(na, pv[i]);
  scanner->ls_inc_sites = na;
}
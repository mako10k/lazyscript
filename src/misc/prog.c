#include "misc/prog.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lsprog {
  const lsexpr_t*  lp_expr;
  const lsarray_t* lp_comments; // array of lscomment_t*
};

struct lsscan {
  const lsprog_t*  ls_prog;
  const char*      ls_filename;
  const char*      ls_sugar_ns;
  const lsarray_t* ls_comments; // array of lscomment_t*
};

const lsprog_t* lsprog_new(const lsexpr_t* expr, const lsarray_t* comments) {
  lsprog_t* prog   = lsmalloc(sizeof(lsprog_t));
  prog->lp_expr    = expr;
  prog->lp_comments = comments;
  return prog;
}

void lsprog_print(FILE* fp, int prec, int indent, const lsprog_t* prog) {
  lsexpr_print(fp, prec, indent, prog->lp_expr);
  lsprintf(fp, 0, ";\n");
}

const lsexpr_t* lsprog_get_expr(const lsprog_t* prog) { return prog->lp_expr; }
const lsarray_t* lsprog_get_comments(const lsprog_t* prog) { return prog->lp_comments; }

void            yyerror(lsloc_t* loc, lsscan_t* scanner, const char* s) {
             (void)scanner;
             lsloc_print(stderr, *loc);
             lsprintf(stderr, 0, "%s\n", s);
}

lsscan_t* lsscan_new(const char* filename) {
  lsscan_t* scanner    = lsmalloc(sizeof(lsscan_t));
  scanner->ls_prog     = NULL;
  scanner->ls_filename = filename;
  scanner->ls_sugar_ns = "prelude";
  scanner->ls_comments = NULL;
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
  if (!scanner) return;
  lscomment_t* c = lsmalloc(sizeof(lscomment_t));
  c->lc_loc = loc;
  c->lc_text = text;
  if (!scanner->ls_comments) {
    scanner->ls_comments = lsarray_new(1, c);
  } else {
    scanner->ls_comments = lsarray_push((lsarray_t*)scanner->ls_comments, (void*)c);
  }
}

const lsarray_t* lsscan_take_comments(lsscan_t* scanner) {
  const lsarray_t* out = scanner ? scanner->ls_comments : NULL;
  if (scanner) scanner->ls_comments = NULL;
  return out;
}
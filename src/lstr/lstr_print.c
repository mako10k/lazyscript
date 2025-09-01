#include "lstr.h"
#include "lstr_internal.h"
#include "thunk/thunk_bin.h"
#include "thunk/thunk_bin.h"
#include <stdio.h>
static void print_pat_inline(FILE* out, const lstr_pat_t* p){
  if (!p){ fprintf(out, "(wild)"); return; }
  switch (p->kind){
    case LSTR_PAT_WILDCARD: fprintf(out, "(wild)"); break;
    case LSTR_PAT_VAR: fprintf(out, "(var %.*s)", (int)lsstr_get_len(p->as.var_name), lsstr_get_buf(p->as.var_name)); break;
    case LSTR_PAT_CONSTR: {
      fprintf(out, "(constr %.*s", (int)lsstr_get_len(p->as.constr.name), lsstr_get_buf(p->as.constr.name));
      for (size_t i=0;i<p->as.constr.argc;i++){ fputc(' ', out); print_pat_inline(out, p->as.constr.args[i]); }
      fputc(')', out);
    } break;
    case LSTR_PAT_INT: fprintf(out, "(int %lld)", p->as.i); break;
    case LSTR_PAT_STR: fprintf(out, "(str "); lsstr_print(out, 0, 0, p->as.s); fputc(')', out); break;
    case LSTR_PAT_AS:  fprintf(out, "(as "), print_pat_inline(out, p->as.as_pat.ref_pat), fputc(' ', out), print_pat_inline(out, p->as.as_pat.inner), fputc(')', out); break;
    case LSTR_PAT_OR:  fprintf(out, "(or "), print_pat_inline(out, p->as.or_pat.left), fputc(' ', out), print_pat_inline(out, p->as.or_pat.right), fputc(')', out); break;
    case LSTR_PAT_CARET: fprintf(out, "(caret "), print_pat_inline(out, p->as.caret_inner), fputc(')', out); break;
    default: fprintf(out, "(pat ?)");
  }
}

static void print_val_inline(FILE* out, const lstr_val_t* v);
static void print_expr_inline(FILE* out, const lstr_expr_t* e);

static void print_val_inline(FILE* out, const lstr_val_t* v) {
  switch (v->kind) {
    case LSTR_VAL_INT:    fprintf(out, "(int %lld)", v->as.i); break;
    case LSTR_VAL_STR:    fprintf(out, "(str "); lsstr_print(out, 0, 0, v->as.s); fputc(')', out); break;
    case LSTR_VAL_REF:    fprintf(out, "(ref %.*s)", (int)lsstr_get_len(v->as.ref_name), lsstr_get_buf(v->as.ref_name)); break;
    case LSTR_VAL_CONSTR: {
      fprintf(out, "(constr %.*s", (int)lsstr_get_len(v->as.constr.name), lsstr_get_buf(v->as.constr.name));
      for (size_t i=0;i<v->as.constr.argc;i++){ fputc(' ', out); print_val_inline(out, v->as.constr.args[i]); }
      fputc(')', out);
    } break;
    case LSTR_VAL_LAM: {
      fprintf(out, "(lam ");
      print_pat_inline(out, v->as.lam.param);
      fputc(' ', out);
      print_expr_inline(out, v->as.lam.body);
      fputc(')', out);
    } break;
    case LSTR_VAL_BOTTOM: {
      fprintf(out, "(bottom ");
      if (v->as.bottom.msg) fprintf(out, "%s", v->as.bottom.msg);
      for (size_t i=0;i<v->as.bottom.relc;i++){ fputc(' ', out); print_val_inline(out, v->as.bottom.related[i]); }
      fputc(')', out);
    } break;
    case LSTR_VAL_CHOICE: {
      fprintf(out, "(choice ");
      const char* kind = (v->as.choice.kind == LSTB_CK_LAMBDA) ? "lambda" :
                         (v->as.choice.kind == LSTB_CK_EXPR)   ? "expr"   :
                         (v->as.choice.kind == LSTB_CK_CATCH)  ? "catch"  : "?";
      fprintf(out, "%s ", kind);
      print_expr_inline(out, v->as.choice.left);
      fputc(' ', out);
      print_expr_inline(out, v->as.choice.right);
      fputc(')', out);
    } break;
    default:
      fprintf(out, "(val ?)");
  }
}

static void print_expr_inline(FILE* out, const lstr_expr_t* e) {
  switch (e->kind) {
    case LSTR_EXP_VAL:
      print_val_inline(out, e->as.v);
      break;
    case LSTR_EXP_APP: {
      fprintf(out, "(app ");
      print_expr_inline(out, e->as.app.func);
      for (size_t i=0;i<e->as.app.argc;i++){ fputc(' ', out); print_expr_inline(out, e->as.app.args[i]); }
      fputc(')', out);
    } break;
    default:
      fprintf(out, "(expr ?)");
  }
}

void lstr_print(FILE* out, int indent, const lstr_prog_t* p) {
  (void)indent;
  fprintf(out, "; LSTR v1\n");
  for (size_t i=0;i<p->rootc;i++) {
    print_expr_inline(out, p->roots[i]);
    fputc('\n', out);
  }
}

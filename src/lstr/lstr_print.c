#include "lstr.h"
#include "lstr_internal.h"
#include "common/str.h"
#include <stdio.h>

static void pad(FILE* out, int n){ while(n-->0) fputc(' ', out); }

static void print_val(FILE* out, int indent, const lstr_val_t* v);
static void print_expr(FILE* out, int indent, const lstr_expr_t* e) {
  switch (e->kind) {
    case LSTR_EXP_VAL:
      print_val(out, indent, e->as.v);
      break;
    case LSTR_EXP_APP: {
  pad(out, indent); fprintf(out, "(app ");
  print_expr(out, 0, e->as.app.func);
  for (size_t i=0;i<e->as.app.argc;i++){ fputc(' ', out); print_expr(out, 0, e->as.app.args[i]); }
  fprintf(out, ")\n");
    } break;
    default:
      pad(out, indent); fprintf(out, "(expr ?)\n");
  }
}

static void print_val(FILE* out, int indent, const lstr_val_t* v) {
  pad(out, indent);
  switch (v->kind) {
    case LSTR_VAL_INT:    fprintf(out, "(int %lld)\n", v->as.i); break;
    case LSTR_VAL_STR:    fprintf(out, "(str "); lsstr_print(out, 0, 0, v->as.s); fprintf(out, ")\n"); break;
    case LSTR_VAL_REF:    fprintf(out, "(ref %.*s)\n", (int)lsstr_get_len(v->as.ref_name), lsstr_get_buf(v->as.ref_name)); break;
    case LSTR_VAL_CONSTR: {
      fprintf(out, "(constr %.*s", (int)lsstr_get_len(v->as.constr.name), lsstr_get_buf(v->as.constr.name));
      for (size_t i=0;i<v->as.constr.argc;i++){ fputc(' ', out); print_val(out, 0, v->as.constr.args[i]); }
      fprintf(out, ")\n");
    } break;
    case LSTR_VAL_LAM: {
      fprintf(out, "(lam ");
      // param (minimal)
      if (v->as.lam.param && v->as.lam.param->kind==LSTR_PAT_VAR) {
        fprintf(out, "(var %.*s) ", (int)lsstr_get_len(v->as.lam.param->as.var_name), lsstr_get_buf(v->as.lam.param->as.var_name));
      } else {
        fprintf(out, "(wild) ");
      }
      print_expr(out, 0, v->as.lam.body);
      fprintf(out, ")\n");
    } break;
    case LSTR_VAL_BOTTOM: {
      fprintf(out, "(bottom ");
      if (v->as.bottom.msg) fprintf(out, "%s", v->as.bottom.msg);
      for (size_t i=0;i<v->as.bottom.relc;i++){ fputc(' ', out); print_val(out, 0, v->as.bottom.related[i]); }
      fprintf(out, ")\n");
    } break;
    case LSTR_VAL_CHOICE: {
      fprintf(out, "(choice ");
      // kind is opaque int for now
      fprintf(out, "%d ", (int)0);
      print_expr(out, 0, v->as.choice.left);
      print_expr(out, 0, v->as.choice.right);
      fprintf(out, ")\n");
    } break;
    default:
      fprintf(out, "(val ?)\n");
  }
}

void lstr_print(FILE* out, int indent, const lstr_prog_t* p) {
  fprintf(out, "; LSTR v1\n");
  for (size_t i=0;i<p->rootc;i++) {
    print_expr(out, indent, p->roots[i]);
  }
}

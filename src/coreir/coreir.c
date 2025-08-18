#include "coreir/coreir.h"
#include "common/malloc.h"
#include "common/io.h"
#include "misc/prog.h"
#include "expr/expr.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/elambda.h"
#include "common/ref.h"
#include "common/int.h"
#include "common/str.h"

struct lscir_prog {
  const lsprog_t *ast;
};

static void print_value_like(FILE *fp, int indent, const lsexpr_t *expr);

static void print_list_open(FILE *fp, int indent, const char *head) {
  lsprintf(fp, indent, "(%s", head);
}

static void print_list_close(FILE *fp) { fprintf(fp, ")"); }

static void print_space(FILE *fp) { fputc(' ', fp); }

static void print_ealge(FILE *fp, int indent, const lsealge_t *ealge) {
  const lsstr_t *name = lsealge_get_constr(ealge);
  print_list_open(fp, indent, lsstr_get_buf(name));
  lssize_t argc = lsealge_get_argc(ealge);
  const lsexpr_t *const *args = lsealge_get_args(ealge);
  for (lssize_t i = 0; i < argc; i++) {
    print_space(fp);
    print_value_like(fp, 0, args[i]);
  }
  print_list_close(fp);
}

static void print_value_like(FILE *fp, int indent, const lsexpr_t *expr) {
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_INT:
    print_list_open(fp, indent, "int");
    print_space(fp);
    lsint_print(fp, LSPREC_LOWEST, 0, lsexpr_get_int(expr));
    print_list_close(fp);
    return;
  case LSETYPE_STR:
    print_list_open(fp, indent, "str");
    print_space(fp);
    lsstr_print(fp, LSPREC_LOWEST, 0, lsexpr_get_str(expr));
    print_list_close(fp);
    return;
  case LSETYPE_REF: {
    print_list_open(fp, indent, "ref");
    print_space(fp);
    const lsref_t *r = lsexpr_get_ref(expr);
    lsref_print(fp, LSPREC_LOWEST, 0, r);
    print_list_close(fp);
    return;
  }
  case LSETYPE_ALGE:
    print_ealge(fp, indent, lsexpr_get_alge(expr));
    return;
  case LSETYPE_LAMBDA: {
    const lselambda_t *lam = lsexpr_get_lambda(expr);
    print_list_open(fp, indent, "lam");
    print_space(fp);
    // print pattern with existing printer
    lspat_print(fp, LSPREC_LOWEST, 0, lselambda_get_param(lam));
    print_space(fp);
    // body as expression-form
    const lsexpr_t *body = lselambda_get_body(lam);
    print_value_like(fp, 0, body);
    print_list_close(fp);
    return;
  }
  case LSETYPE_APPL: {
    const lseappl_t *ap = lsexpr_get_appl(expr);
    print_list_open(fp, indent, "app");
    print_space(fp);
    print_value_like(fp, 0, lseappl_get_func(ap));
    lssize_t argc = lseappl_get_argc(ap);
    const lsexpr_t *const *args = lseappl_get_args(ap);
    for (lssize_t i = 0; i < argc; i++) {
      print_space(fp);
      print_value_like(fp, 0, args[i]);
    }
    print_list_close(fp);
    return;
  }
  case LSETYPE_CHOICE:
  case LSETYPE_CLOSURE:
    // Fallback to existing pretty printers
    lsexpr_print(fp, LSPREC_LOWEST, indent, expr);
    return;
  }
}

const lscir_prog_t *lscir_lower_prog(const lsprog_t *prog) {
  lscir_prog_t *cir = lsmalloc(sizeof(lscir_prog_t));
  cir->ast = prog;
  return cir;
}

void lscir_print(FILE *fp, int indent, const lscir_prog_t *cir) {
  // Temporary: just print the surface AST under a [CoreIR] heading
  fprintf(fp, "; CoreIR dump (temporary)\n");
  const lsprog_t *ast = cir->ast;
  if (ast) {
    const lsexpr_t *e = lsprog_get_expr(ast);
    if (e) {
      print_value_like(fp, indent, e);
      fprintf(fp, "\n");
    } else {
      fprintf(fp, "<empty>\n");
    }
  } else {
    fprintf(fp, "<null>\n");
  }
}

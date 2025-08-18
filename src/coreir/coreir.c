// Clean implementation starts here
#include <stdlib.h>

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
#include "pat/pat.h"

struct lscir_prog {
  const lsprog_t *ast;           // fallback printing用
  const lscir_expr_t *root;      // Lower済みIR（無ければNULL）
};

/* ---------- AST fallback printer (S式) ---------- */
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
    lspat_print(fp, LSPREC_LOWEST, 0, lselambda_get_param(lam));
    print_space(fp);
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
    lsexpr_print(fp, LSPREC_LOWEST, indent, expr);
    return;
  }
}

/* ---------- IR builders ---------- */
static const lscir_value_t *mk_val_int(long long v) {
  lscir_value_t *x = lsmalloc(sizeof(*x));
  x->kind = LCIR_VAL_INT; x->ival = v; return x;
}
static const lscir_value_t *mk_val_str(const char *s) {
  lscir_value_t *x = lsmalloc(sizeof(*x));
  x->kind = LCIR_VAL_STR; x->sval = s; return x;
}
static const lscir_value_t *mk_val_var(const char *name) {
  lscir_value_t *x = lsmalloc(sizeof(*x));
  x->kind = LCIR_VAL_VAR; x->var = name; return x;
}
static const lscir_value_t *mk_val_constr(const char *name, int argc, const lscir_value_t *const *args) {
  lscir_value_t *x = lsmalloc(sizeof(*x));
  x->kind = LCIR_VAL_CONSTR; x->constr.name = name; x->constr.argc = argc; x->constr.args = args; return x;
}
static const lscir_value_t *mk_val_lam(const char *param, const lscir_expr_t *body) {
  lscir_value_t *x = lsmalloc(sizeof(*x));
  x->kind = LCIR_VAL_LAM; x->lam.param = param; x->lam.body = body; return x;
}

static const lscir_expr_t *mk_exp_val(const lscir_value_t *v) {
  lscir_expr_t *e = lsmalloc(sizeof(*e));
  e->kind = LCIR_EXP_VAL; e->v = v; return e;
}
static const lscir_expr_t *mk_exp_app(const lscir_value_t *f, int argc, const lscir_value_t *const *args) {
  lscir_expr_t *e = lsmalloc(sizeof(*e));
  e->kind = LCIR_EXP_APP; e->app.func = f; e->app.argc = argc; e->app.args = args; return e;
}

/* ---------- Lowering (happy path) ---------- */
static const char *extract_ref_name(const lsref_t *r) {
  const lsstr_t *nm = lsref_get_name(r);
  return lsstr_get_buf(nm);
}

static const char *extract_pat_name(const lspat_t *p) {
  if (!p) return "_";
  if (lspat_get_type(p) == LSPTYPE_REF) {
    return lsstr_get_buf(lsref_get_name(lspat_get_ref(p)));
  }
  char *buf = NULL; size_t len = 0; FILE *fp = lsopen_memstream_gc(&buf, &len);
  lspat_print(fp, LSPREC_LOWEST, 0, p); fclose(fp);
  return buf ? buf : "_";
}

static const lscir_value_t *lower_value(const lsexpr_t *e);

static const lscir_value_t *lower_alge_value(const lsealge_t *ea) {
  const lsstr_t *c = lsealge_get_constr(ea);
  int argc = (int)lsealge_get_argc(ea);
  const lsexpr_t *const *eargs = lsealge_get_args(ea);
  const lscir_value_t **args = NULL;
  if (argc > 0) {
    const lscir_value_t **tmp = lsmalloc(sizeof(const lscir_value_t*) * argc);
    for (int i = 0; i < argc; i++) {
      const lscir_value_t *vi = lower_value(eargs[i]);
      if (!vi) return NULL;
      ((const lscir_value_t**)tmp)[i] = vi;
    }
    args = tmp;
  }
  return mk_val_constr(lsstr_get_buf(c), argc, args);
}

static const lscir_value_t *lower_value(const lsexpr_t *e) {
  switch (lsexpr_get_type(e)) {
  case LSETYPE_INT: {
    char *buf = NULL; size_t len = 0; FILE *fp = lsopen_memstream_gc(&buf, &len);
    lsint_print(fp, LSPREC_LOWEST, 0, lsexpr_get_int(e)); fclose(fp);
    long long v = atoll(buf ? buf : "0");
    return mk_val_int(v);
  }
  case LSETYPE_STR:
    return mk_val_str(lsstr_get_buf(lsexpr_get_str(e)));
  case LSETYPE_REF:
    return mk_val_var(extract_ref_name(lsexpr_get_ref(e)));
  case LSETYPE_ALGE:
    return lower_alge_value(lsexpr_get_alge(e));
  case LSETYPE_LAMBDA: {
    const lselambda_t *lam = lsexpr_get_lambda(e);
    const char *param = extract_pat_name(lselambda_get_param(lam));
    const lsexpr_t *body = lselambda_get_body(lam);
    const lscir_value_t *bv = lower_value(body);
    if (bv) return mk_val_lam(param, mk_exp_val(bv));
    return NULL;
  }
  default:
    return NULL;
  }
}

static const lscir_expr_t *lower_expr(const lsexpr_t *e) {
  if (!e) return NULL;
  switch (lsexpr_get_type(e)) {
  case LSETYPE_APPL: {
    const lseappl_t *ap = lsexpr_get_appl(e);
    const lscir_value_t *f = lower_value(lseappl_get_func(ap));
    if (!f) return NULL;
    int argc = (int)lseappl_get_argc(ap);
    const lsexpr_t *const *eargs = lseappl_get_args(ap);
    const lscir_value_t **args = NULL;
    if (argc > 0) {
      const lscir_value_t **tmp = lsmalloc(sizeof(const lscir_value_t*) * argc);
      for (int i = 0; i < argc; i++) {
        const lscir_value_t *vi = lower_value(eargs[i]);
        if (!vi) return NULL;
        ((const lscir_value_t**)tmp)[i] = vi;
      }
      args = tmp;
    }
    return mk_exp_app(f, argc, args);
  }
  default: {
    const lscir_value_t *v = lower_value(e);
    return v ? mk_exp_val(v) : NULL;
  }
  }
}

const lscir_prog_t *lscir_lower_prog(const lsprog_t *prog) {
  lscir_prog_t *cir = lsmalloc(sizeof(lscir_prog_t));
  cir->ast = prog;
  cir->root = NULL;
  const lsexpr_t *e = lsprog_get_expr(prog);
  if (e) {
    cir->root = lower_expr(e);
  }
  return cir;
}

/* ---------- IR printer ---------- */
static void cir_print_val(FILE *ofp, int ind, const lscir_value_t *v);
static void cir_print_expr(FILE *ofp, int ind, const lscir_expr_t *e2);

static void cir_print_val(FILE *ofp, int ind, const lscir_value_t *v) {
  switch (v->kind) {
  case LCIR_VAL_INT: fprintf(ofp, "(int %lld)", v->ival); return;
  case LCIR_VAL_STR: fprintf(ofp, "(str \""); fputs(v->sval, ofp); fprintf(ofp, "\")"); return;
  case LCIR_VAL_VAR: fprintf(ofp, "(var %s)", v->var); return;
  case LCIR_VAL_CONSTR: {
    lsprintf(ofp, ind, "(%s", v->constr.name);
    for (int i = 0; i < v->constr.argc; i++) { fputc(' ', ofp); cir_print_val(ofp, 0, v->constr.args[i]); }
    fputc(')', ofp); return;
  }
  case LCIR_VAL_LAM:
    lsprintf(ofp, ind, "(lam %s ", v->lam.param);
    cir_print_expr(ofp, 0, v->lam.body);
    fputc(')', ofp); return;
  }
}

static void cir_print_expr(FILE *ofp, int ind, const lscir_expr_t *e2) {
  switch (e2->kind) {
  case LCIR_EXP_VAL: cir_print_val(ofp, ind, e2->v); return;
  case LCIR_EXP_APP:
    lsprintf(ofp, ind, "(app ");
    cir_print_val(ofp, 0, e2->app.func);
    for (int i = 0; i < e2->app.argc; i++) { fputc(' ', ofp); cir_print_val(ofp, 0, e2->app.args[i]); }
    fputc(')', ofp); return;
  default:
    fprintf(ofp, "<unimpl>"); return;
  }
}

void lscir_print(FILE *fp, int indent, const lscir_prog_t *cir) {
  fprintf(fp, "; CoreIR dump (temporary)\n");
  if (cir->root) {
    cir_print_expr(fp, indent, cir->root);
    fprintf(fp, "\n");
  } else if (cir->ast) {
    const lsexpr_t *e = lsprog_get_expr(cir->ast);
    if (e) { print_value_like(fp, indent, e); fprintf(fp, "\n"); }
    else { fprintf(fp, "<empty>\n"); }
  } else {
    fprintf(fp, "<null>\n");
  }
}

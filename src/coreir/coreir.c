// Clean implementation starts here
#include <stdlib.h>
#include <string.h>

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
static const lscir_expr_t *mk_exp_let(const char *var, const lscir_expr_t *bind, const lscir_expr_t *body) {
  lscir_expr_t *e = lsmalloc(sizeof(*e));
  e->kind = LCIR_EXP_LET; e->let1.var = var; e->let1.bind = bind; e->let1.body = body; return e;
}
static const lscir_expr_t *mk_exp_if(const lscir_value_t *cond, const lscir_expr_t *then_e, const lscir_expr_t *else_e) {
  lscir_expr_t *e = lsmalloc(sizeof(*e));
  e->kind = LCIR_EXP_IF; e->ife.cond = cond; e->ife.then_e = then_e; e->ife.else_e = else_e; return e;
}
static const lscir_expr_t *mk_exp_token(void) {
  lscir_expr_t *e = lsmalloc(sizeof(*e));
  e->kind = LCIR_EXP_TOKEN; return e;
}
static const lscir_expr_t *mk_exp_effapp(const lscir_value_t *f, const lscir_value_t *tok, int argc, const lscir_value_t *const *args) {
  lscir_expr_t *e = lsmalloc(sizeof(*e));
  e->kind = LCIR_EXP_EFFAPP; e->effapp.func = f; e->effapp.token = tok; e->effapp.argc = argc; e->effapp.args = args; return e;
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

static const lscir_expr_t *lower_expr(const lsexpr_t *e);
static const lscir_value_t *lower_value(const lsexpr_t *e);

/* gensym utility */
static const char *cir_gensym(const char *pfx) {
  static int s_gsym = 0;
  char buf[64];
  int n = snprintf(buf, sizeof(buf), "%s%d", pfx, ++s_gsym);
  char *s = lsmalloc((size_t)n + 1);
  memcpy(s, buf, (size_t)n + 1);
  return s;
}

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
  const lscir_expr_t *be = lower_expr(body);
  if (be) return mk_val_lam(param, be);
  return NULL;
  }
  default:
    return NULL;
  }
}

static const lscir_expr_t *lower_closure(const lseclosure_t *cl) {
  // Build let x1 = rhs1 in let x2 = rhs2 in ... body
  const lsexpr_t *body_ast = lseclosure_get_expr(cl);
  const lscir_expr_t *body = lower_expr(body_ast);
  if (!body) return NULL;
  lssize_t n = lseclosure_get_bindc(cl);
  const lsbind_t *const *bs = lseclosure_get_binds(cl);
  for (lssize_t i = n; i > 0; i--) {
    const lsbind_t *b = bs[i - 1];
    const lspat_t *lhs = lsbind_get_lhs(b);
    if (!lhs || lspat_get_type(lhs) != LSPTYPE_REF) {
      // 未対応の複合パターン: 今はロールバック
      return NULL;
    }
    const char *vname = lsstr_get_buf(lsref_get_name(lspat_get_ref(lhs)));
    const lsexpr_t *rhs_ast = lsbind_get_rhs(b);
    const lscir_expr_t *rhs = lower_expr(rhs_ast);
    if (!rhs) return NULL;
    body = mk_exp_let(vname, rhs, body);
  }
  return body;
}

static const lscir_expr_t *lower_expr(const lsexpr_t *e) {
  if (!e) return NULL;
  switch (lsexpr_get_type(e)) {
  case LSETYPE_APPL: {
    const lseappl_t *ap = lsexpr_get_appl(e);
    const lsexpr_t *func_e = lseappl_get_func(ap);
    int argc = (int)lseappl_get_argc(ap);
    const lsexpr_t *const *eargs = lseappl_get_args(ap);

    // Special-case: ifthenelse cond then else  →  LCIR_EXP_IF
    if (lsexpr_get_type(func_e) == LSETYPE_REF) {
      const char *fname = extract_ref_name(lsexpr_get_ref(func_e));
      if (fname && strcmp(fname, "ifthenelse") == 0 && argc == 3) {
        const lsexpr_t *cond_e = eargs[0];
        const lsexpr_t *then_ast = eargs[1];
        const lsexpr_t *else_ast = eargs[2];
        const lscir_value_t *cv = lower_value(cond_e);
        const char *cn = NULL;
        if (!cv) { cn = cir_gensym("c$"); cv = mk_val_var(cn); }
        const lscir_expr_t *core = mk_exp_if(cv, lower_expr(then_ast), lower_expr(else_ast));
        if (cn) core = mk_exp_let(cn, lower_expr(cond_e), core);
        return core;
      }
    }

    // Special-case: (~prelude sym) args...  → EffApp(sym, token, args...)
    if (lsexpr_get_type(func_e) == LSETYPE_APPL) {
      const lseappl_t *ap2 = lsexpr_get_appl(func_e);
      const lsexpr_t *f2 = lseappl_get_func(ap2);
      if (lsexpr_get_type(f2) == LSETYPE_REF && strcmp(extract_ref_name(lsexpr_get_ref(f2)), "prelude") == 0) {
        if ((int)lseappl_get_argc(ap2) == 1) {
          const lsexpr_t *a0 = lseappl_get_args(ap2)[0];
          const char *sym = NULL;
          if (lsexpr_get_type(a0) == LSETYPE_REF) {
            sym = extract_ref_name(lsexpr_get_ref(a0));
          } else if (lsexpr_get_type(a0) == LSETYPE_ALGE) {
            const lsealge_t *al = lsexpr_get_alge(a0);
            if (lsealge_get_argc(al) == 0) sym = lsstr_get_buf(lsealge_get_constr(al));
          }
          if (sym) {
            // 暫定: println/exit のみ効果扱い
            if (sym && (strcmp(sym, "println") == 0 || strcmp(sym, "exit") == 0)) {
              // 値化した引数を作る（ANF）
              const lscir_value_t **vals = NULL;
              const char **lnames = NULL; const lsexpr_t **lexprs = NULL; int letc = 0;
              if (argc > 0) vals = lsmalloc(sizeof(const lscir_value_t*) * argc);
              for (int i = 0; i < argc; i++) {
                const lscir_value_t *vi = lower_value(eargs[i]);
                if (vi) { vals[i] = vi; }
                else {
                  const char *tn = cir_gensym("a$");
                  vals[i] = mk_val_var(tn);
                  const char **nn = lsmalloc(sizeof(const char*) * (letc + 1));
                  const lsexpr_t **ee = lsmalloc(sizeof(const lsexpr_t*) * (letc + 1));
                  for (int k = 0; k < letc; k++) { nn[k] = lnames[k]; ee[k] = lexprs[k]; }
                  nn[letc] = tn; ee[letc] = eargs[i]; letc++; lnames = nn; lexprs = ee;
                }
              }
              const lscir_value_t *fv = mk_val_var(sym);
              // token を導入
              const char *tokn = cir_gensym("E$");
              const lscir_value_t *tokv = mk_val_var(tokn);
              const lscir_expr_t *core = mk_exp_effapp(fv, tokv, argc, vals);
              // 引数の let を巻く
              for (int i = letc - 1; i >= 0; i--) core = mk_exp_let(lnames[i], lower_expr(lexprs[i]), core);
              // token let を先頭に
              core = mk_exp_let(tokn, mk_exp_token(), core);
              return core;
            }
          }
        }
      }
    }

    // Collect value args, recording non-values to be let-bound later
    const lscir_value_t **vals = NULL;
    const char **let_names = NULL;
    const lsexpr_t **let_exprs = NULL;
    int letc = 0;
    if (argc > 0) {
      vals = lsmalloc(sizeof(const lscir_value_t*) * argc);
    }
    for (int i = 0; i < argc; i++) {
      const lscir_value_t *vi = lower_value(eargs[i]);
      if (vi) {
        vals[i] = vi;
      } else {
        // non-value: create temp var and record let binding
  const char *tmpv = cir_gensym("a$");
        vals[i] = mk_val_var(tmpv);
        // grow arrays
        const char **nn = lsmalloc(sizeof(const char*) * (letc + 1));
        const lsexpr_t **ee = lsmalloc(sizeof(const lsexpr_t*) * (letc + 1));
        for (int k = 0; k < letc; k++) { nn[k] = let_names[k]; ee[k] = let_exprs[k]; }
        nn[letc] = tmpv; ee[letc] = eargs[i]; letc++;
        let_names = nn; let_exprs = ee;
      }
    }

    // Function value or let-binding if non-value
    const lscir_value_t *fv = lower_value(func_e);
    const char *f_tmp = NULL;
    if (!fv) {
      f_tmp = cir_gensym("f$");
      fv = mk_val_var(f_tmp);
    }

    // Build core app
    const lscir_expr_t *core = mk_exp_app(fv, argc, vals);

    // Wrap lets for args in reverse order
    for (int i = letc - 1; i >= 0; i--) {
      core = mk_exp_let(let_names[i], lower_expr(let_exprs[i]), core);
    }
    // Wrap let for function if needed
    if (f_tmp != NULL) {
      core = mk_exp_let(f_tmp, lower_expr(func_e), core);
    }
    return core;
  }
  case LSETYPE_CHOICE: {
    // Lower e1 | e2  ≈  (app (var |) v1 v2) with ANF lets for non-values
    const lsechoice_t *ch = lsexpr_get_choice(e);
    const lsexpr_t *l = lsechoice_get_left(ch);
    const lsexpr_t *r = lsechoice_get_right(ch);

    const lscir_value_t *vl = lower_value(l);
    const lscir_value_t *vr = lower_value(r);
    const char *ln = NULL, *rn = NULL;
    if (!vl) { ln = cir_gensym("c$"); vl = mk_val_var(ln); }
    if (!vr) { rn = cir_gensym("c$"); vr = mk_val_var(rn); }

  const lscir_value_t *f = mk_val_var("|");
  const lscir_value_t **args_arr = lsmalloc(sizeof(const lscir_value_t*) * 2);
  args_arr[0] = vl; args_arr[1] = vr;
  const lscir_expr_t *core = mk_exp_app(f, 2, args_arr);
    if (rn) core = mk_exp_let(rn, lower_expr(r), core);
    if (ln) core = mk_exp_let(ln, lower_expr(l), core);
    return core;
  }
  case LSETYPE_CLOSURE: {
    const lseclosure_t *cl = lsexpr_get_closure(e);
    return lower_closure(cl);
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
  case LCIR_EXP_LET:
    lsprintf(ofp, ind, "(let %s ", e2->let1.var);
    cir_print_expr(ofp, 0, e2->let1.bind);
    fputc(' ', ofp);
    cir_print_expr(ofp, 0, e2->let1.body);
    fputc(')', ofp); return;
  case LCIR_EXP_IF:
    lsprintf(ofp, ind, "(if ");
    cir_print_val(ofp, 0, e2->ife.cond);
    fputc(' ', ofp);
    cir_print_expr(ofp, 0, e2->ife.then_e);
    fputc(' ', ofp);
    cir_print_expr(ofp, 0, e2->ife.else_e);
    fputc(')', ofp); return;
  case LCIR_EXP_EFFAPP:
    lsprintf(ofp, ind, "(effapp ");
    cir_print_val(ofp, 0, e2->effapp.func);
    fputc(' ', ofp);
    cir_print_val(ofp, 0, e2->effapp.token);
    for (int i = 0; i < e2->effapp.argc; i++) { fputc(' ', ofp); cir_print_val(ofp, 0, e2->effapp.args[i]); }
    fputc(')', ofp); return;
  case LCIR_EXP_TOKEN:
    lsprintf(ofp, ind, "(token)"); return;
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

/* ---------- Effects validator ---------- */
typedef struct eff_scope {
  int has_token; // 0/1: token が導入済みか
} eff_scope_t;

static int cir_validate_expr(FILE *efp, const lscir_expr_t *e, eff_scope_t sc);

static void cir_val_repr(char *buf, size_t n, const lscir_value_t *v) {
  if (!buf || n == 0) return;
  if (!v) { snprintf(buf, n, "<null>"); return; }
  switch (v->kind) {
  case LCIR_VAL_INT: snprintf(buf, n, "int %lld", v->ival); return;
  case LCIR_VAL_STR: snprintf(buf, n, "str ..."); return;
  case LCIR_VAL_VAR: snprintf(buf, n, "var %s", v->var ? v->var : "?"); return;
  case LCIR_VAL_CONSTR: snprintf(buf, n, "%s(...)", v->constr.name ? v->constr.name : "<constr>"); return;
  case LCIR_VAL_LAM: snprintf(buf, n, "lam %s", v->lam.param ? v->lam.param : "_"); return;
  }
  snprintf(buf, n, "<val>");
}

static int cir_validate_val(FILE *efp, const lscir_value_t *v, eff_scope_t sc) {
  (void)efp; (void)sc;
  // Lam の中は別作用域で検査（token は引き継がない前提）
  if (v && v->kind == LCIR_VAL_LAM) {
    eff_scope_t inner = {0};
    return cir_validate_expr(efp, v->lam.body, inner);
  }
  return 0;
}

static int cir_validate_expr(FILE *efp, const lscir_expr_t *e, eff_scope_t sc) {
  if (!e) return 0;
  switch (e->kind) {
  case LCIR_EXP_VAL:
    return cir_validate_val(efp, e->v, sc);
  case LCIR_EXP_LET: {
    // bind は現スコープ、body は bind の後のスコープ
    int errs = cir_validate_expr(efp, e->let1.bind, sc);
    eff_scope_t sc2 = sc;
    if (e->let1.bind && e->let1.bind->kind == LCIR_EXP_TOKEN)
      sc2.has_token = 1;
    errs += cir_validate_expr(efp, e->let1.body, sc2);
    return errs;
  }
  case LCIR_EXP_APP: {
    int errs = 0;
    errs += cir_validate_val(efp, e->app.func, sc);
    for (int i = 0; i < e->app.argc; i++) errs += cir_validate_val(efp, e->app.args[i], sc);
    return errs;
  }
  case LCIR_EXP_IF: {
    int errs = 0;
    errs += cir_validate_val(efp, e->ife.cond, sc);
    errs += cir_validate_expr(efp, e->ife.then_e, sc);
    errs += cir_validate_expr(efp, e->ife.else_e, sc);
    return errs;
  }
  case LCIR_EXP_EFFAPP: {
    int errs = 0;
    if (!sc.has_token) {
  char fbuf[64]; cir_val_repr(fbuf, sizeof(fbuf), e->effapp.func);
  fprintf(efp, "E: EffApp(%s): requires in-scope token (introduce via (token) let).\n", fbuf);
      errs++;
    }
    errs += cir_validate_val(efp, e->effapp.func, sc);
    if (e->effapp.token->kind != LCIR_VAL_VAR) {
  char tbuf[64]; cir_val_repr(tbuf, sizeof(tbuf), e->effapp.token);
  fprintf(efp, "E: EffApp token must be a variable, got: %s.\n", tbuf);
      errs++;
    }
    for (int i = 0; i < e->effapp.argc; i++) errs += cir_validate_val(efp, e->effapp.args[i], sc);
    return errs;
  }
  case LCIR_EXP_TOKEN:
    return 0; // 導入点そのもの
  }
  return 0;
}

int lscir_validate_effects(FILE *errfp, const lscir_prog_t *cir) {
  if (!cir || !cir->root) return 0;
  eff_scope_t sc = {0};
  return cir_validate_expr(errfp, cir->root, sc);
}

/* ---------- Minimal Core IR evaluator (smoke) ---------- */
typedef enum {
  RV_INT,
  RV_STR,
  RV_UNIT,
  RV_TOKEN,
  RV_LAM,
  RV_SYMBOL,
  RV_NATFUN,
} rv_kind_t;

typedef struct rv rv_t;

typedef struct env_bind {
  const char *name;
  const rv_t *val;
  struct env_bind *next;
} env_bind_t;

struct rv {
  rv_kind_t kind;
  union {
    long long ival;
    const char *sval;
    struct {
      const char *param;
      const lscir_expr_t *body;
      env_bind_t *env; // captured environment
    } lam;
    const char *sym;
    struct {
      const char *name;
      int arity;
      int capc; // captured count
      const rv_t *const *caps; // captured args
    } nf;
  };
};

static env_bind_t *env_push(env_bind_t *env, const char *name, const rv_t *v) {
  env_bind_t *b = lsmalloc(sizeof(env_bind_t));
  b->name = name; b->val = v; b->next = env; return b;
}

static const rv_t *env_lookup(env_bind_t *env, const char *name) {
  for (env_bind_t *p = env; p; p = p->next) if (strcmp(p->name, name) == 0) return p->val;
  return NULL;
}

static const rv_t *rv_int(long long x) { rv_t *v = lsmalloc(sizeof(rv_t)); v->kind = RV_INT; v->ival = x; return v; }
static const rv_t *rv_str(const char *s) { rv_t *v = lsmalloc(sizeof(rv_t)); v->kind = RV_STR; v->sval = s; return v; }
static const rv_t *rv_unit(void) { static rv_t u = { .kind = RV_UNIT }; return &u; }
static const rv_t *rv_tok(void) { static rv_t t = { .kind = RV_TOKEN }; return &t; }
static const rv_t *rv_sym(const char *s) { rv_t *v = lsmalloc(sizeof(rv_t)); v->kind = RV_SYMBOL; v->sym = s; return v; }
static const rv_t *rv_lam(const char *p, const lscir_expr_t *b, env_bind_t *env) {
  rv_t *v = lsmalloc(sizeof(rv_t)); v->kind = RV_LAM; v->lam.param = p; v->lam.body = b; v->lam.env = env; return v;
}
static const rv_t *rv_natfun(const char *name, int arity, int capc, const rv_t *const *caps) {
  rv_t *v = lsmalloc(sizeof(rv_t)); v->kind = RV_NATFUN; v->nf.name = name; v->nf.arity = arity; v->nf.capc = capc; v->nf.caps = caps; return v;
}

static const rv_t *eval_value(FILE *outfp, const lscir_value_t *v, env_bind_t *env);
static const rv_t *eval_expr(FILE *outfp, const lscir_expr_t *e, env_bind_t *env);

static const rv_t *eval_value(FILE *outfp, const lscir_value_t *v, env_bind_t *env) {
  (void)outfp;
  switch (v->kind) {
  case LCIR_VAL_INT: return rv_int(v->ival);
  case LCIR_VAL_STR: return rv_str(v->sval);
  case LCIR_VAL_VAR: {
  const rv_t *found = env_lookup(env, v->var);
  if (found) return found;
  // Builtin natfuns (curried)
  if (strcmp(v->var, "add") == 0) return rv_natfun("add", 2, 0, NULL);
  if (strcmp(v->var, "sub") == 0) return rv_natfun("sub", 2, 0, NULL);
  // Treat others as symbols (e.g., true/false)
  return rv_sym(v->var);
  }
  case LCIR_VAL_CONSTR: {
    if (strcmp(v->constr.name, "()") == 0 && v->constr.argc == 0) return rv_unit();
    // Fallback: represent as symbol name
    return rv_sym(v->constr.name);
  }
  case LCIR_VAL_LAM:
    // Capture current env to form a closure
    return rv_lam(v->lam.param, v->lam.body, env);
  }
  return rv_unit();
}

static int truthy(const rv_t *v) {
  if (!v) return 0;
  switch (v->kind) {
  case RV_INT: return v->ival != 0;
  case RV_SYMBOL: return (strcmp(v->sym, "true") == 0) ? 1 : (strcmp(v->sym, "false") == 0 ? 0 : 1);
  case RV_UNIT: return 0;
  default: return 1;
  }
}

static void print_value(FILE *outfp, const rv_t *v) {
  switch (v->kind) {
  case RV_UNIT: fprintf(outfp, "()\n"); return;
  case RV_INT: fprintf(outfp, "%lld\n", v->ival); return;
  case RV_STR: fprintf(outfp, "%s\n", v->sval); return;
  case RV_SYMBOL: fprintf(outfp, "%s\n", v->sym); return;
  case RV_LAM: fprintf(outfp, "<lam>\n"); return;
  case RV_TOKEN: fprintf(outfp, "<token>\n"); return;
  case RV_NATFUN: fprintf(outfp, "<fun %s/%d:%d>\n", v->nf.name, v->nf.arity, v->nf.capc); return;
  }
}

static const rv_t *natfun_apply_one(FILE *outfp, const rv_t *nf, const rv_t *arg) {
  if (!nf || nf->kind != RV_NATFUN) return NULL;
  const char *name = nf->nf.name; int ar = nf->nf.arity; int cap = nf->nf.capc;
  // If not enough args yet, return a new curried function with appended capture
  if (cap + 1 < ar) {
    const rv_t **caps2 = lsmalloc(sizeof(rv_t*) * (cap + 1));
    for (int i = 0; i < cap; i++) caps2[i] = nf->nf.caps ? nf->nf.caps[i] : NULL;
    caps2[cap] = arg;
    return rv_natfun(name, ar, cap + 1, caps2);
  }
  // Reaching full arity now
  if (strcmp(name, "add") == 0 || strcmp(name, "sub") == 0) {
    const rv_t *a = (cap >= 1 && nf->nf.caps) ? nf->nf.caps[0] : NULL;
    const rv_t *b = arg;
    if (!a || !b || a->kind != RV_INT || b->kind != RV_INT) return rv_unit();
    long long res = (strcmp(name, "add") == 0) ? (a->ival + b->ival) : (a->ival - b->ival);
    return rv_int(res);
  }
  if (strcmp(name, "println") == 0) {
    // println x  ==> print then unit
    if (arg->kind == RV_STR) fprintf(outfp, "%s\n", arg->sval);
    else if (arg->kind == RV_INT) fprintf(outfp, "%lld\n", arg->ival);
    else if (arg->kind == RV_SYMBOL) fprintf(outfp, "%s\n", arg->sym);
    return rv_unit();
  }
  if (strcmp(name, "exit") == 0) {
    long long code = (arg->kind == RV_INT) ? arg->ival : 1;
    exit(code == 0 ? 0 : 1);
  }
  if (strcmp(name, "return") == 0) {
    // return x ==> x
    return arg;
  }
  if (strcmp(name, "chain") == 0) {
    // chain a f  ==> f ()
    // (a は既に評価済み値として cap[0] にある想定。効果は let で順序づけ済み)
    const rv_t *f = arg;
    if (f && f->kind == RV_LAM) {
      env_bind_t *env2 = env_push(f->lam.env, f->lam.param, rv_unit());
      return eval_expr(outfp, f->lam.body, env2);
    }
    return f ? f : rv_unit();
  }
  return rv_unit();
}

static const rv_t *eval_expr(FILE *outfp, const lscir_expr_t *e, env_bind_t *env) {
  if (!e) return rv_unit();
  switch (e->kind) {
  case LCIR_EXP_VAL:
    return eval_value(outfp, e->v, env);
  case LCIR_EXP_LET: {
    const rv_t *bv = eval_expr(outfp, e->let1.bind, env);
    env_bind_t *env2 = env_push(env, e->let1.var, bv);
    return eval_expr(outfp, e->let1.body, env2);
  }
  case LCIR_EXP_APP: {
    // Prelude dispatcher: (app (var prelude) <sym>)  ==> natfun(sym)
    if (e->app.func->kind == LCIR_VAL_VAR && strcmp(e->app.func->var, "prelude") == 0 && e->app.argc == 1) {
      const rv_t *sv = eval_value(outfp, e->app.args[0], env);
      if (sv && sv->kind == RV_SYMBOL) {
        const char *s = sv->sym;
        if (strcmp(s, "println") == 0) return rv_natfun("println", 1, 0, NULL);
        if (strcmp(s, "exit") == 0)    return rv_natfun("exit", 1, 0, NULL);
        if (strcmp(s, "chain") == 0)   return rv_natfun("chain", 2, 0, NULL);
        if (strcmp(s, "return") == 0)  return rv_natfun("return", 1, 0, NULL);
      }
      return rv_unit();
    }
    // Evaluate function value first
    const rv_t *rf = eval_value(outfp, e->app.func, env);
    // Curry: apply each argument from left to right
    for (int i = 0; i < e->app.argc; i++) {
      const rv_t *av = eval_value(outfp, e->app.args[i], env);
      if (!rf) return rv_unit();
      if (rf->kind == RV_LAM) {
        env_bind_t *env2 = env_push(rf->lam.env, rf->lam.param, av);
        rf = eval_expr(outfp, rf->lam.body, env2);
        continue;
      }
      if (rf->kind == RV_NATFUN) {
        rf = natfun_apply_one(outfp, rf, av);
        continue;
      }
      // Applying a plain symbol? Try lifting builtins to natfun
      if (rf->kind == RV_SYMBOL && (strcmp(rf->sym, "add") == 0 || strcmp(rf->sym, "sub") == 0)) {
        rf = natfun_apply_one(outfp, rv_natfun(rf->sym, 2, 0, NULL), av);
        continue;
      }
      // Not applicable
      return rv_unit();
    }
    return rf ? rf : rv_unit();
  }
  case LCIR_EXP_IF: {
    const rv_t *cv = eval_value(outfp, e->ife.cond, env);
    if (truthy(cv)) return eval_expr(outfp, e->ife.then_e, env);
    else return eval_expr(outfp, e->ife.else_e, env);
  }
  case LCIR_EXP_EFFAPP: {
    // Token presence is assumed by validator; ignore here.
    if (e->effapp.func->kind == LCIR_VAL_VAR && strcmp(e->effapp.func->var, "println") == 0) {
      if (e->effapp.argc != 1) return rv_unit();
      const rv_t *av = eval_value(outfp, e->effapp.args[0], env);
      if (av->kind == RV_STR) fprintf(outfp, "%s\n", av->sval);
      else if (av->kind == RV_INT) fprintf(outfp, "%lld\n", av->ival);
      else if (av->kind == RV_SYMBOL) fprintf(outfp, "%s\n", av->sym);
      return rv_unit();
    }
    if (e->effapp.func->kind == LCIR_VAL_VAR && strcmp(e->effapp.func->var, "exit") == 0) {
      if (e->effapp.argc != 1) return rv_unit();
      const rv_t *av = eval_value(outfp, e->effapp.args[0], env);
      long long code = (av->kind == RV_INT) ? av->ival : 1;
      exit(code == 0 ? 0 : 1);
    }
    return rv_unit();
  }
  case LCIR_EXP_TOKEN:
    return rv_tok();
  }
  return rv_unit();
}

int lscir_eval(FILE *outfp, const lscir_prog_t *cir) {
  if (!cir || !cir->root) return 0;
  env_bind_t *env = NULL;
  const rv_t *res = eval_expr(outfp, cir->root, env);
  print_value(outfp, res);
  return 0;
}

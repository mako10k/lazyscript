#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "coreir/cir_internal.h"
#include "coreir/coreir.h"
#include "common/malloc.h"
#include "common/io.h"
#include "misc/prog.h"
#include "expr/expr.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/elambda.h"
#include "expr/enslit.h"
#include "common/ref.h"
#include "common/int.h"
#include "common/str.h"
#include "pat/pat.h"

// ---- pretty printing helpers for original AST fallback ----
static void print_value_like(FILE* fp, int indent, const lsexpr_t* expr);

static void print_list_open(FILE* fp, int indent, const char* head) {
  lsprintf(fp, indent, "(%s", head);
}
static void print_list_close(FILE* fp) { fprintf(fp, ")"); }
static void print_space(FILE* fp) { fputc(' ', fp); }

static void print_ealge(FILE* fp, int indent, const lsealge_t* ealge) {
  const lsstr_t* name = lsealge_get_constr(ealge);
  print_list_open(fp, indent, lsstr_get_buf(name));
  lssize_t               argc = lsealge_get_argc(ealge);
  const lsexpr_t* const* args = lsealge_get_args(ealge);
  for (lssize_t i = 0; i < argc; i++) {
    print_space(fp);
    print_value_like(fp, 0, args[i]);
  }
  print_list_close(fp);
}

static void print_value_like(FILE* fp, int indent, const lsexpr_t* expr) {
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
    const lsref_t* r = lsexpr_get_ref(expr);
    lsref_print(fp, LSPREC_LOWEST, 0, r);
    print_list_close(fp);
    return;
  }
  case LSETYPE_ALGE:
    print_ealge(fp, indent, lsexpr_get_alge(expr));
    return;
  case LSETYPE_LAMBDA: {
    const lselambda_t* lam = lsexpr_get_lambda(expr);
    print_list_open(fp, indent, "lam");
    print_space(fp);
    lspat_print(fp, LSPREC_LOWEST, 0, lselambda_get_param(lam));
    print_space(fp);
    const lsexpr_t* body = lselambda_get_body(lam);
    print_value_like(fp, 0, body);
    print_list_close(fp);
    return;
  }
  case LSETYPE_APPL: {
    const lseappl_t* ap = lsexpr_get_appl(expr);
    print_list_open(fp, indent, "app");
    print_space(fp);
    print_value_like(fp, 0, lseappl_get_func(ap));
    lssize_t               argc = lseappl_get_argc(ap);
    const lsexpr_t* const* args = lseappl_get_args(ap);
    for (lssize_t i = 0; i < argc; i++) {
      print_space(fp);
      print_value_like(fp, 0, args[i]);
    }
    print_list_close(fp);
    return;
  }
  case LSETYPE_CHOICE:
  case LSETYPE_CLOSURE:
  case LSETYPE_NSLIT:
  case LSETYPE_SYMBOL:
    lsexpr_print(fp, LSPREC_LOWEST, indent, expr);
    return;
  }
}

// ---- constructors ----
static const lscir_value_t* mk_val_int(long long v) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_INT;
  x->ival          = v;
  return x;
}
static const lscir_value_t* mk_val_str(const char* s) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_STR;
  x->sval          = s;
  return x;
}
static const lscir_value_t* mk_val_var(const char* name) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_VAR;
  x->var           = name;
  return x;
}
static const lscir_value_t* mk_val_constr(const char* name, int argc,
                                          const lscir_value_t* const* args) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_CONSTR;
  x->constr.name   = name;
  x->constr.argc   = argc;
  x->constr.args   = args;
  return x;
}
static const lscir_value_t* mk_val_lam(const char* param, const lscir_expr_t* body) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_LAM;
  x->lam.param     = param;
  x->lam.body      = body;
  return x;
}

static const lscir_value_t* mk_val_nslit(int n, const char* const* names,
                                        const lscir_value_t* const* vals) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_NSLIT;
  x->nslit.count   = n;
  x->nslit.names   = names;
  x->nslit.vals    = vals;
  return x;
}

static const lscir_expr_t* mk_exp_val(const lscir_value_t* v) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_VAL;
  e->v            = v;
  return e;
}
static const lscir_expr_t* mk_exp_app(const lscir_value_t* f, int argc,
                                      const lscir_value_t* const* args) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_APP;
  e->app.func     = f;
  e->app.argc     = argc;
  e->app.args     = args;
  return e;
}
static const lscir_expr_t* mk_exp_let(const char* var, const lscir_expr_t* bind,
                                      const lscir_expr_t* body) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_LET;
  e->let1.var     = var;
  e->let1.bind    = bind;
  e->let1.body    = body;
  return e;
}
static const lscir_expr_t* mk_exp_if(const lscir_value_t* cond, const lscir_expr_t* then_e,
                                     const lscir_expr_t* else_e) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_IF;
  e->ife.cond     = cond;
  e->ife.then_e   = then_e;
  e->ife.else_e   = else_e;
  return e;
}
static const lscir_expr_t* mk_exp_token(void) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_TOKEN;
  return e;
}
static const lscir_expr_t* mk_exp_effapp(const lscir_value_t* f, const lscir_value_t* tok, int argc,
                                         const lscir_value_t* const* args) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_EFFAPP;
  e->effapp.func  = f;
  e->effapp.token = tok;
  e->effapp.argc  = argc;
  e->effapp.args  = args;
  return e;
}

// ---- lowering ----
static const lscir_expr_t*  lower_expr(const lsexpr_t* e);
static const lscir_value_t* lower_value(const lsexpr_t* e);

static const char* extract_ref_name(const lsref_t* r) { return lsstr_get_buf(lsref_get_name(r)); }
static const char* extract_pat_name(const lspat_t* p) {
  if (!p)
    return "_";
  if (lspat_get_type(p) == LSPTYPE_REF)
    return lsstr_get_buf(lsref_get_name(lspat_get_ref(p)));
  char*  buf = NULL;
  size_t len = 0;
  FILE*  fp  = lsopen_memstream_gc(&buf, &len);
  lspat_print(fp, LSPREC_LOWEST, 0, p);
  fclose(fp);
  return buf ? buf : "_";
}

static const char* cir_gensym(const char* pfx) {
  static int s_gsym = 0;
  char       buf[64];
  int        n = snprintf(buf, sizeof(buf), "%s%d", pfx, ++s_gsym);
  char*      s = lsmalloc((size_t)n + 1);
  memcpy(s, buf, (size_t)n + 1);
  return s;
}

static const lscir_value_t* lower_alge_value(const lsealge_t* ea) {
  const lsstr_t*         c     = lsealge_get_constr(ea);
  int                    argc  = (int)lsealge_get_argc(ea);
  const lsexpr_t* const* eargs = lsealge_get_args(ea);
  const lscir_value_t**  args  = NULL;
  if (argc > 0) {
    const lscir_value_t** tmp = lsmalloc(sizeof(const lscir_value_t*) * argc);
    for (int i = 0; i < argc; i++) {
      const lscir_value_t* vi = lower_value(eargs[i]);
      if (!vi)
        return NULL;
      ((const lscir_value_t**)tmp)[i] = vi;
    }
    args = tmp;
  }
  return mk_val_constr(lsstr_get_buf(c), argc, args);
}

static const lscir_value_t* lower_value(const lsexpr_t* e) {
  switch (lsexpr_get_type(e)) {
  case LSETYPE_INT: {
    char*  buf = NULL;
    size_t len = 0;
    FILE*  fp  = lsopen_memstream_gc(&buf, &len);
    lsint_print(fp, LSPREC_LOWEST, 0, lsexpr_get_int(e));
    fclose(fp);
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
    const lselambda_t*  lam   = lsexpr_get_lambda(e);
    const char*         param = extract_pat_name(lselambda_get_param(lam));
    const lsexpr_t*     body  = lselambda_get_body(lam);
    const lscir_expr_t* be    = lower_expr(body);
    if (be)
      return mk_val_lam(param, be);
    return NULL;
  }
  case LSETYPE_NSLIT: {
    const lsenslit_t* ns = lsexpr_get_nslit(e);
    lssize_t          n  = lsenslit_get_count(ns);
    const char**      names = NULL;
    const lscir_value_t** vals = NULL;
    if (n > 0) {
      names = lsmalloc(sizeof(char*) * (size_t)n);
      vals  = lsmalloc(sizeof(lscir_value_t*) * (size_t)n);
      for (lssize_t i = 0; i < n; i++) {
        const char* nb = lsstr_get_buf(lsenslit_get_name(ns, i));
        if (nb && nb[0] == '.') nb++;
        names[i] = nb;
        const lscir_value_t* vi = lower_value(lsenslit_get_expr(ns, i));
        if (!vi)
          return NULL;
        ((const lscir_value_t**)vals)[i] = vi;
      }
    }
    return mk_val_nslit((int)n, names, vals);
  }
  default:
    return NULL;
  }
}

static const lscir_expr_t* lower_closure(const lseclosure_t* cl) {
  const lsexpr_t*     body_ast = lseclosure_get_expr(cl);
  const lscir_expr_t* body     = lower_expr(body_ast);
  if (!body)
    return NULL;
  lssize_t               n  = lseclosure_get_bindc(cl);
  const lsbind_t* const* bs = lseclosure_get_binds(cl);
  for (lssize_t i = n; i > 0; i--) {
    const lsbind_t* b   = bs[i - 1];
    const lspat_t*  lhs = lsbind_get_lhs(b);
    if (!lhs || lspat_get_type(lhs) != LSPTYPE_REF)
      return NULL;
    const char*         vname = lsstr_get_buf(lsref_get_name(lspat_get_ref(lhs)));
    const lsexpr_t*     rhs_a = lsbind_get_rhs(b);
    const lscir_expr_t* rhs   = lower_expr(rhs_a);
    if (!rhs)
      return NULL;
    body = mk_exp_let(vname, rhs, body);
  }
  return body;
}

static const lscir_expr_t* lower_expr(const lsexpr_t* e) {
  if (!e)
    return NULL;
  switch (lsexpr_get_type(e)) {
  case LSETYPE_APPL: {
    const lseappl_t*       ap     = lsexpr_get_appl(e);
    const lsexpr_t*        func_e = lseappl_get_func(ap);
    int                    argc   = (int)lseappl_get_argc(ap);
    const lsexpr_t* const* eargs  = lseappl_get_args(ap);
    if (lsexpr_get_type(func_e) == LSETYPE_REF) {
      const char* fname = extract_ref_name(lsexpr_get_ref(func_e));
      if (fname && strcmp(fname, "ifthenelse") == 0 && argc == 3) {
        const lsexpr_t*      cond_e = eargs[0];
        const lsexpr_t*      then_a = eargs[1];
        const lsexpr_t*      else_a = eargs[2];
        const lscir_value_t* cv     = lower_value(cond_e);
        const char*          cn     = NULL;
        if (!cv) {
          cn = cir_gensym("c$");
          cv = mk_val_var(cn);
        }
        const lscir_expr_t* core = mk_exp_if(cv, lower_expr(then_a), lower_expr(else_a));
        if (cn)
          core = mk_exp_let(cn, lower_expr(cond_e), core);
        return core;
      }
    }
    if (lsexpr_get_type(func_e) == LSETYPE_APPL) {
      const lseappl_t* ap2 = lsexpr_get_appl(func_e);
      const lsexpr_t*  f2  = lseappl_get_func(ap2);
  // TODO: Avoid silent prelude flattening.
  // This block recognizes (~prelude .env .sym) and emits (var sym). This is a temporary lowering
  // for bootstrapping and must be replaced by explicit IR imports emission and reader-bound registry.
  int is_prelude_head = 0;
      // Case A: (app (var prelude) a0)
      if (lsexpr_get_type(f2) == LSETYPE_REF && strcmp(extract_ref_name(lsexpr_get_ref(f2)), "prelude") == 0) {
        is_prelude_head = 1;
      }
      // Case B: (app (app (var prelude) (.env)) sym) i.e., ((prelude .env) .sym)
      else if (lsexpr_get_type(f2) == LSETYPE_APPL) {
        const lseappl_t* ap_head = lsexpr_get_appl(f2);
        if ((int)lseappl_get_argc(ap_head) == 1) {
          const lsexpr_t* f3 = lseappl_get_func(ap_head);
          if (lsexpr_get_type(f3) == LSETYPE_REF && strcmp(extract_ref_name(lsexpr_get_ref(f3)), "prelude") == 0) {
            const lsexpr_t* a0h = lseappl_get_args(ap_head)[0];
            if ((lsexpr_get_type(a0h) == LSETYPE_SYMBOL && strcmp(lsstr_get_buf(lsexpr_get_symbol(a0h)), ".env") == 0) ||
                (lsexpr_get_type(a0h) == LSETYPE_ALGE && lsealge_get_argc(lsexpr_get_alge(a0h)) == 0 && strcmp(lsstr_get_buf(lsealge_get_constr(lsexpr_get_alge(a0h))), ".env") == 0)) {
              is_prelude_head = 2; // two-step env
            }
          }
        }
      }
      if (is_prelude_head) {
        if ((int)lseappl_get_argc(ap2) == 1) {
          const lsexpr_t* a0  = lseappl_get_args(ap2)[0];
          const char*     sym = NULL;
          if (is_prelude_head == 2) {
            // ((prelude .env) .sym) form: a0 is the .sym
            if (lsexpr_get_type(a0) == LSETYPE_SYMBOL) {
              const char* s = lsstr_get_buf(lsexpr_get_symbol(a0));
              sym = (s && s[0] == '.') ? (s + 1) : s;
            } else if (lsexpr_get_type(a0) == LSETYPE_ALGE) {
              const lsealge_t* al3 = lsexpr_get_alge(a0);
              if (lsealge_get_argc(al3) == 0) {
                const char* s = lsstr_get_buf(lsealge_get_constr(al3));
                sym = (s && s[0] == '.') ? (s + 1) : s;
              }
            }
          } else if (lsexpr_get_type(a0) == LSETYPE_REF) {
            sym = extract_ref_name(lsexpr_get_ref(a0));
          } else if (lsexpr_get_type(a0) == LSETYPE_ALGE) {
            const lsealge_t* al = lsexpr_get_alge(a0);
            if (lsealge_get_argc(al) == 0)
              sym = lsstr_get_buf(lsealge_get_constr(al));
          } else if (lsexpr_get_type(a0) == LSETYPE_APPL) {
            // Support (~prelude .env .sym)
            const lseappl_t* ap3 = lsexpr_get_appl(a0);
            if ((int)lseappl_get_argc(ap3) == 1) {
              const lsexpr_t* envfn  = lseappl_get_func(ap3);
              const lsexpr_t* symarg = lseappl_get_args(ap3)[0];
              if (lsexpr_get_type(envfn) == LSETYPE_APPL) {
                const lseappl_t* ap_env = lsexpr_get_appl(envfn);
                if ((int)lseappl_get_argc(ap_env) == 1) {
                  const lsexpr_t* envkey = lseappl_get_args(ap_env)[0];
                  int             is_env = 0;
                  if (lsexpr_get_type(envkey) == LSETYPE_SYMBOL) {
                    const char* k = lsstr_get_buf(lsexpr_get_symbol(envkey));
                    is_env        = (k && strcmp(k, ".env") == 0);
                  } else if (lsexpr_get_type(envkey) == LSETYPE_ALGE) {
                    const lsealge_t* al2 = lsexpr_get_alge(envkey);
                    if (lsealge_get_argc(al2) == 0) {
                      const char* k = lsstr_get_buf(lsealge_get_constr(al2));
                      is_env        = (k && strcmp(k, ".env") == 0);
                    }
                  }
                  if (is_env) {
                    if (lsexpr_get_type(symarg) == LSETYPE_SYMBOL) {
                      const char* s = lsstr_get_buf(lsexpr_get_symbol(symarg));
                      if (s && s[0] == '.')
                        sym = s + 1;
                      else
                        sym = s;
                    } else if (lsexpr_get_type(symarg) == LSETYPE_ALGE) {
                      const lsealge_t* al3 = lsexpr_get_alge(symarg);
                      if (lsealge_get_argc(al3) == 0) {
                        const char* s = lsstr_get_buf(lsealge_get_constr(al3));
                        if (s && s[0] == '.')
                          sym = s + 1;
                        else
                          sym = s;
                      }
                    }
                  }
                }
              }
            }
          }
          // Effects for println/exit are not special-cased here. Use explicit imports later.
          // TODO: Instead of turning (~prelude .env .sym) into a plain var, emit an import entry
          // and keep reference as var sym bound by the import table.
          if (sym) {
            const lscir_value_t** vals   = NULL;
            const char**          lnames = NULL;
            const lsexpr_t**      lexprs = NULL;
            int                   letc   = 0;
            if (argc > 0)
              vals = lsmalloc(sizeof(const lscir_value_t*) * argc);
            for (int i = 0; i < argc; i++) {
              const lscir_value_t* vi = lower_value(eargs[i]);
              if (vi) {
                vals[i] = vi;
              } else {
                const char*      tmpv = cir_gensym("a$");
                vals[i]                 = mk_val_var(tmpv);
                const char**     nn   = lsmalloc(sizeof(const char*) * (letc + 1));
                const lsexpr_t** ee   = lsmalloc(sizeof(const lsexpr_t*) * (letc + 1));
                for (int k = 0; k < letc; k++) {
                  nn[k] = lnames[k];
                  ee[k] = lexprs[k];
                }
                nn[letc] = tmpv;
                ee[letc] = eargs[i];
                letc++;
                lnames = nn;
                lexprs = ee;
              }
            }
            const lscir_expr_t* core = mk_exp_app(mk_val_var(sym), argc, vals);
            for (int i = letc - 1; i >= 0; i--)
              core = mk_exp_let(lnames[i], lower_expr(lexprs[i]), core);
            return core;
          }
        }
      }
    }
    // Generic application lowering with let-introduction
    const lscir_value_t** vals      = NULL;
    const char**          let_names = NULL;
    const lsexpr_t**      let_exprs = NULL;
    int                   letc      = 0;
    if (argc > 0)
      vals = lsmalloc(sizeof(const lscir_value_t*) * argc);
    for (int i = 0; i < argc; i++) {
      const lscir_value_t* vi = lower_value(eargs[i]);
      if (vi) {
        vals[i] = vi;
      } else {
        const char*      tmpv = cir_gensym("a$");
        vals[i]                 = mk_val_var(tmpv);
        const char**     nn   = lsmalloc(sizeof(const char*) * (letc + 1));
        const lsexpr_t** ee   = lsmalloc(sizeof(const lsexpr_t*) * (letc + 1));
        for (int k = 0; k < letc; k++) {
          nn[k] = let_names[k];
          ee[k] = let_exprs[k];
        }
        nn[letc]     = tmpv;
        ee[letc]     = eargs[i];
        letc++;
        let_names    = nn;
        let_exprs    = ee;
      }
    }
    const lscir_value_t* fv    = lower_value(func_e);
    const char*          f_tmp = NULL;
    if (!fv) {
      f_tmp = cir_gensym("f$");
      fv    = mk_val_var(f_tmp);
    }
    const lscir_expr_t* core = mk_exp_app(fv, argc, vals);
    for (int i = letc - 1; i >= 0; i--)
      core = mk_exp_let(let_names[i], lower_expr(let_exprs[i]), core);
    if (f_tmp != NULL)
      core = mk_exp_let(f_tmp, lower_expr(func_e), core);
    return core;
  }
  case LSETYPE_CHOICE: {
    const lsechoice_t*   ch = lsexpr_get_choice(e);
    const lsexpr_t*      l  = lsechoice_get_left(ch);
    const lsexpr_t*      r  = lsechoice_get_right(ch);
    const lscir_value_t* vl = lower_value(l);
    const lscir_value_t* vr = lower_value(r);
    const char *         ln = NULL, *rn = NULL;
    if (!vl) {
      ln = cir_gensym("c$");
      vl = mk_val_var(ln);
    }
    if (!vr) {
      rn = cir_gensym("c$");
      vr = mk_val_var(rn);
    }
    const lscir_value_t*  f        = mk_val_var("|");
    const lscir_value_t** args_arr = lsmalloc(sizeof(const lscir_value_t*) * 2);
    args_arr[0]                    = vl;
    args_arr[1]                    = vr;
    const lscir_expr_t* core       = mk_exp_app(f, 2, args_arr);
    if (rn)
      core = mk_exp_let(rn, lower_expr(r), core);
    if (ln)
      core = mk_exp_let(ln, lower_expr(l), core);
    return core;
  }
  case LSETYPE_CLOSURE:
    return lower_closure(lsexpr_get_closure(e));
  default: {
    const lscir_value_t* v = lower_value(e);
    return v ? mk_exp_val(v) : NULL;
  }
  }
}

const lscir_prog_t* lscir_lower_prog(const lsprog_t* prog) {
  lscir_prog_t* cir = lsmalloc(sizeof(lscir_prog_t));
  cir->ast          = prog;
  cir->root         = NULL;
  const lsexpr_t* e = lsprog_get_expr(prog);
  if (e)
    cir->root = lower_expr(e);
  return cir;
}

// ---- printing Core IR ----
static void cir_print_val(FILE* ofp, int ind, const lscir_value_t* v);
static void cir_print_expr(FILE* ofp, int ind, const lscir_expr_t* e2);

static void cir_print_val(FILE* ofp, int ind, const lscir_value_t* v) {
  if (!v) {
    fprintf(ofp, "<null-val>");
    return;
  }
  switch (v->kind) {
  case LCIR_VAL_INT:
    fprintf(ofp, "(int %lld)", v->ival);
    return;
  case LCIR_VAL_STR:
    fprintf(ofp, "(str \"");
    fputs(v->sval, ofp);
    fprintf(ofp, "\")");
    return;
  case LCIR_VAL_VAR:
    fprintf(ofp, "(var %s)", v->var);
    return;
  case LCIR_VAL_CONSTR: {
    lsprintf(ofp, ind, "(%s", v->constr.name);
    for (int i = 0; i < v->constr.argc; i++) {
      fputc(' ', ofp);
      cir_print_val(ofp, 0, v->constr.args[i]);
    }
    fputc(')', ofp);
    return;
  }
  case LCIR_VAL_LAM:
    lsprintf(ofp, ind, "(lam %s ", v->lam.param);
    cir_print_expr(ofp, 0, v->lam.body);
    fputc(')', ofp);
    return;
  case LCIR_VAL_NSLIT: {
    lsprintf(ofp, ind, "(nslit");
    for (int i = 0; i < v->nslit.count; i++) {
      fputc(' ', ofp);
      lsprintf(ofp, 0, "(%s ", v->nslit.names[i]);
      cir_print_val(ofp, 0, v->nslit.vals[i]);
      fputc(')', ofp);
    }
    fputc(')', ofp);
    return;
  }
  }
}

static void cir_print_expr(FILE* ofp, int ind, const lscir_expr_t* e2) {
  if (!e2) {
    fprintf(ofp, "<null-expr>");
    return;
  }
  switch (e2->kind) {
  case LCIR_EXP_VAL:
    cir_print_val(ofp, ind, e2->v);
    return;
  case LCIR_EXP_APP:
    lsprintf(ofp, ind, "(app ");
    cir_print_val(ofp, 0, e2->app.func);
    for (int i = 0; i < e2->app.argc; i++) {
      fputc(' ', ofp);
      cir_print_val(ofp, 0, e2->app.args[i]);
    }
    fputc(')', ofp);
    return;
  case LCIR_EXP_LET:
    lsprintf(ofp, ind, "(let %s ", e2->let1.var);
    cir_print_expr(ofp, 0, e2->let1.bind);
    fputc(' ', ofp);
    cir_print_expr(ofp, 0, e2->let1.body);
    fputc(')', ofp);
    return;
  case LCIR_EXP_IF:
    lsprintf(ofp, ind, "(if ");
    cir_print_val(ofp, 0, e2->ife.cond);
    fputc(' ', ofp);
    cir_print_expr(ofp, 0, e2->ife.then_e);
    fputc(' ', ofp);
    cir_print_expr(ofp, 0, e2->ife.else_e);
    fputc(')', ofp);
    return;
  case LCIR_EXP_EFFAPP:
    lsprintf(ofp, ind, "(effapp ");
    cir_print_val(ofp, 0, e2->effapp.func);
    fputc(' ', ofp);
    cir_print_val(ofp, 0, e2->effapp.token);
    for (int i = 0; i < e2->effapp.argc; i++) {
      fputc(' ', ofp);
      cir_print_val(ofp, 0, e2->effapp.args[i]);
    }
    fputc(')', ofp);
    return;
  case LCIR_EXP_TOKEN:
    lsprintf(ofp, ind, "(token)");
    return;
  default:
    fprintf(ofp, "<unimpl>");
    return;
  }
}

void lscir_print(FILE* fp, int indent, const lscir_prog_t* cir) {
  fprintf(fp, "; CoreIR dump (temporary)\n");
  if (cir->root) {
    cir_print_expr(fp, indent, cir->root);
    fprintf(fp, "\n");
  } else if (cir->ast) {
    const lsexpr_t* e = lsprog_get_expr(cir->ast);
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

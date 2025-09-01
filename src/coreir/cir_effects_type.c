#include <stdio.h>
#include <string.h>

#include "coreir/cir_internal.h"
#include "coreir/coreir.h"

// ---------------- Effect discipline validator ----------------

typedef struct eff_scope {
  int has_token;
} eff_scope_t;
static int  cir_validate_expr(FILE* efp, const lscir_expr_t* e, eff_scope_t sc);

static void cir_val_repr(char* buf, size_t n, const lscir_value_t* v) {
  if (!buf || n == 0)
    return;
  if (!v) {
    snprintf(buf, n, "<null>");
    return;
  }
  switch (v->kind) {
  case LCIR_VAL_INT:
    snprintf(buf, n, "int %lld", v->ival);
    return;
  case LCIR_VAL_STR:
    snprintf(buf, n, "str ...");
    return;
  case LCIR_VAL_VAR:
    snprintf(buf, n, "var %s", v->var ? v->var : "?");
    return;
  case LCIR_VAL_CONSTR:
    snprintf(buf, n, "%s(...)", v->constr.name ? v->constr.name : "<constr>");
    return;
  case LCIR_VAL_LAM:
    snprintf(buf, n, "lam %s", v->lam.param ? v->lam.param : "_");
    return;
  case LCIR_VAL_NSLIT:
    snprintf(buf, n, "nslit{...}");
    return;
  }
  snprintf(buf, n, "<val>");
}

static int cir_validate_val(FILE* efp, const lscir_value_t* v, eff_scope_t sc) {
  if (!v)
    return 0;
  switch (v->kind) {
  case LCIR_VAL_LAM: {
    eff_scope_t inner = (eff_scope_t){ 0 };
    return cir_validate_expr(efp, v->lam.body, inner);
  }
  case LCIR_VAL_NSLIT: {
    int errs = 0;
    for (int i = 0; i < v->nslit.count; i++)
      errs += cir_validate_val(efp, v->nslit.vals[i], sc);
    return errs;
  }
  default:
    return 0;
  }
}

static int cir_validate_expr(FILE* efp, const lscir_expr_t* e, eff_scope_t sc) {
  if (!e)
    return 0;
  switch (e->kind) {
  case LCIR_EXP_VAL:
    return cir_validate_val(efp, e->v, sc);
  case LCIR_EXP_LET: {
    int         errs = cir_validate_expr(efp, e->let1.bind, sc);
    eff_scope_t sc2  = sc;
    if (e->let1.bind && e->let1.bind->kind == LCIR_EXP_TOKEN)
      sc2.has_token = 1;
    errs += cir_validate_expr(efp, e->let1.body, sc2);
    return errs;
  }
  case LCIR_EXP_APP: {
    int errs = 0;
    errs += cir_validate_val(efp, e->app.func, sc);
    for (int i = 0; i < e->app.argc; i++)
      errs += cir_validate_val(efp, e->app.args[i], sc);
    return errs;
  }
  case LCIR_EXP_IF: {
    int errs = 0;
    errs += cir_validate_val(efp, e->ife.cond, sc);
    errs += cir_validate_expr(efp, e->ife.then_e, sc);
    errs += cir_validate_expr(efp, e->ife.else_e, sc);
    return errs;
  }
  case LCIR_EXP_MATCH: {
    int errs = 0;
    errs += cir_validate_val(efp, e->match1.scrut, sc);
    for (int i = 0; i < e->match1.casec; i++) {
      errs += cir_validate_expr(efp, e->match1.cases[i].body, sc);
    }
    return errs;
  }
  case LCIR_EXP_EFFAPP: {
    int errs = 0;
    if (!sc.has_token) {
      char fbuf[64];
      cir_val_repr(fbuf, sizeof(fbuf), e->effapp.func);
      fprintf(efp, "E: EffApp(%s): requires in-scope token (introduce via (token) let).\n", fbuf);
      errs++;
    }
    errs += cir_validate_val(efp, e->effapp.func, sc);
    if (e->effapp.token->kind != LCIR_VAL_VAR) {
      char tbuf[64];
      cir_val_repr(tbuf, sizeof(tbuf), e->effapp.token);
      fprintf(efp, "E: EffApp token must be a variable, got: %s.\n", tbuf);
      errs++;
    }
    for (int i = 0; i < e->effapp.argc; i++)
      errs += cir_validate_val(efp, e->effapp.args[i], sc);
    return errs;
  }
  case LCIR_EXP_TOKEN:
    return 0;
  }
  return 0;
}

int lscir_validate_effects(FILE* errfp, const lscir_prog_t* cir) {
  if (!cir || !cir->root)
    return 0;
  eff_scope_t sc = (eff_scope_t){ 0 };
  return cir_validate_expr(errfp, cir->root, sc);
}

// ---------------- Minimal typecheck (arity + kind warnings) ----------------

static int g_kind_warn  = 1;
static int g_kind_error = 0;
void       lscir_typecheck_set_kind_warn(int warn_enabled) { g_kind_warn = warn_enabled ? 1 : 0; }
void lscir_typecheck_set_kind_error(int error_enabled) { g_kind_error = error_enabled ? 1 : 0; }

static int val_has_effects(const lscir_value_t* v);
static int expr_has_effects(const lscir_expr_t* e, int in_token_scope) {
  if (!e)
    return 0;
  switch (e->kind) {
  case LCIR_EXP_VAL:
    return val_has_effects(e->v);
  case LCIR_EXP_LET: {
    int t = in_token_scope;
    if (e->let1.bind && e->let1.bind->kind == LCIR_EXP_TOKEN)
      t = 1;
    return expr_has_effects(e->let1.bind, in_token_scope) || expr_has_effects(e->let1.body, t);
  }
  case LCIR_EXP_APP: {
    int any = 0;
    for (int i = 0; i < e->app.argc; i++)
      any |= val_has_effects(e->app.args[i]);
    return any;
  }
  case LCIR_EXP_IF:
    return expr_has_effects(e->ife.then_e, in_token_scope) ||
           expr_has_effects(e->ife.else_e, in_token_scope);
  case LCIR_EXP_MATCH: {
    int any = 0;
    for (int i = 0; i < e->match1.casec; i++)
      any |= expr_has_effects(e->match1.cases[i].body, in_token_scope);
    return any;
  }
  case LCIR_EXP_EFFAPP:
    return 1;
  case LCIR_EXP_TOKEN:
    return 0;
  }
  return 0;
}

static int val_has_effects(const lscir_value_t* v) {
  if (v && v->kind == LCIR_VAL_LAM)
    return expr_has_effects(v->lam.body, 0);
  return 0;
}

// TODO: Arity checking for native functions must consult an explicit import/registry table
//       attached to the CIR program, not hard-coded names.
static int natfun_arity(const char* name) {
  (void)name; // unused until registry is introduced
  return -1;  // unknown; do not enforce over-application until registry exists
}

static int expr_check_arity(const lscir_expr_t* e) {
  if (!e)
    return 0;
  switch (e->kind) {
  case LCIR_EXP_VAL:
    return 0;
  case LCIR_EXP_LET:
    return expr_check_arity(e->let1.bind) + expr_check_arity(e->let1.body);
  case LCIR_EXP_APP: {
    int errs = 0;
    if (e->app.func && e->app.func->kind == LCIR_VAL_VAR) {
      int ar = natfun_arity(e->app.func->var);
      // 新仕様: 部分適用は OK。過剰適用のみエラー。ただし現状は未判定（ar < 0）。
      if (ar >= 0 && e->app.argc > ar)
        errs++;
    }
    return errs;
  }
  case LCIR_EXP_IF:
    return expr_check_arity(e->ife.then_e) + expr_check_arity(e->ife.else_e);
  case LCIR_EXP_MATCH: {
    int errs = 0;
    for (int i = 0; i < e->match1.casec; i++)
      errs += expr_check_arity(e->match1.cases[i].body);
    return errs;
  }
  case LCIR_EXP_EFFAPP:
    return 0;
  case LCIR_EXP_TOKEN:
    return 0;
  }
  return 0;
}

int lscir_typecheck(FILE* outfp, const lscir_prog_t* cir) {
  if (!cir || !cir->root) {
    fprintf(outfp, "OK\n");
    return 0;
  }
  // Arity errors first
  int arity_errs = expr_check_arity(cir->root);
  if (arity_errs > 0) {
    fprintf(outfp, "E: type error\n");
    return 1;
  }

  int has_io = expr_has_effects(cir->root, 0);
  if (g_kind_error && has_io) {
    fprintf(stderr, "E: kind error: IO detected in pure context\n");
    return 1;
  }
  if (g_kind_warn && has_io) {
    fprintf(stderr, "W: kind: this term performs IO (effects)\n");
  }
  fprintf(outfp, "OK\n");
  return 0;
}

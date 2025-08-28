#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "coreir/cir_internal.h"
#include "coreir/coreir.h"
#include "common/malloc.h"

// Runtime value kinds
typedef enum {
  RV_INT,
  RV_STR,
  RV_UNIT,
  RV_TOKEN,
  RV_LAM,
  RV_SYMBOL,
  RV_NATFUN,
  RV_CONSTR,
  RV_BOTTOM
} rv_kind_t;

typedef struct rv rv_t;

typedef struct env_bind {
  const char*      name;
  const rv_t*      val;
  struct env_bind* next;
} env_bind_t;

typedef struct eval_ctx {
  env_bind_t* env;
  int         has_token;
} eval_ctx_t;

struct rv {
  rv_kind_t kind;
  union {
    long long   ival;
    const char* sval;
    struct {
      const char*         param;
      const lscir_expr_t* body;
      env_bind_t*         env;
    } lam;
    const char* sym;
    struct {
      const char*        name;
      int                arity;
      int                capc;
      const rv_t* const* caps;
    } nf;
    struct {
      const char*        name;
      int                argc;
      const rv_t* const* args;
    } con;
    struct { // internal bottom representation for runtime evaluator
      const char*        message;
      int                argc;
      const rv_t* const* args;
    } bot;
  };
};

static env_bind_t* env_push(env_bind_t* env, const char* name, const rv_t* v) {
  env_bind_t* b = lsmalloc(sizeof(env_bind_t));
  b->name       = name;
  b->val        = v;
  b->next       = env;
  return b;
}
static const rv_t* env_lookup(env_bind_t* env, const char* name) {
  for (env_bind_t* p = env; p; p = p->next)
    if (strcmp(p->name, name) == 0)
      return p->val;
  return NULL;
}

static const rv_t* rv_int(long long x) {
  rv_t* v = lsmalloc(sizeof(rv_t));
  v->kind = RV_INT;
  v->ival = x;
  return v;
}
static const rv_t* rv_str(const char* s) {
  rv_t* v = lsmalloc(sizeof(rv_t));
  v->kind = RV_STR;
  v->sval = s;
  return v;
}
static const rv_t* rv_unit(void) {
  static rv_t u = { .kind = RV_UNIT };
  return &u;
}
static const rv_t* rv_tok(void) {
  static rv_t t = { .kind = RV_TOKEN };
  return &t;
}
static const rv_t* rv_sym(const char* s) {
  rv_t* v = lsmalloc(sizeof(rv_t));
  v->kind = RV_SYMBOL;
  v->sym  = s;
  return v;
}
static const rv_t* rv_bottom(const char* msg, int argc, const rv_t* const* args) {
  rv_t* v     = lsmalloc(sizeof(rv_t));
  v->kind     = RV_BOTTOM;
  v->bot.message = msg;
  v->bot.argc    = argc;
  v->bot.args    = args;
  return v;
}
static const rv_t* rv_lam(const char* p, const lscir_expr_t* b, env_bind_t* env) {
  rv_t* v      = lsmalloc(sizeof(rv_t));
  v->kind      = RV_LAM;
  v->lam.param = p;
  v->lam.body  = b;
  v->lam.env   = env;
  return v;
}
static const rv_t* rv_natfun(const char* name, int arity, int capc, const rv_t* const* caps) {
  rv_t* v     = lsmalloc(sizeof(rv_t));
  v->kind     = RV_NATFUN;
  v->nf.name  = name;
  v->nf.arity = arity;
  v->nf.capc  = capc;
  v->nf.caps  = caps;
  return v;
}
static const rv_t* rv_constr(const char* name, int argc, const rv_t* const* args) {
  rv_t* v     = lsmalloc(sizeof(rv_t));
  v->kind     = RV_CONSTR;
  v->con.name = name;
  v->con.argc = argc;
  v->con.args = args;
  return v;
}

static const rv_t* eval_value(FILE* outfp, const lscir_value_t* v, eval_ctx_t* ctx);
static const rv_t* eval_expr(FILE* outfp, const lscir_expr_t* e, eval_ctx_t* ctx);
static const rv_t* apply(FILE* outfp, const rv_t* f, int argc, const rv_t* const* args,
                         eval_ctx_t* ctx);

// POLICY NOTE:
//  - Unauthorized fallback is prohibited.
//  - Do not use fixed identifiers (prelude, builtins, internal, env, specific builtin names) in implementation logic.
// TEMP/TODO: Import-based builtin resolution is not implemented yet for Core IR runtime.
//            Until an explicit import/registry exists in the IR and reader, unknown variables must NOT be
//            implicitly resolved to native functions. They remain plain symbols and will not be callable here.
//            Follow-up: Introduce explicit import table in CIR and wire evaluator/typechecker to it.
static const rv_t* eval_value(FILE* outfp, const lscir_value_t* v, eval_ctx_t* ctx) {
  (void)outfp;
  switch (v->kind) {
  case LCIR_VAL_INT:
    return rv_int(v->ival);
  case LCIR_VAL_STR:
    return rv_str(v->sval);
  case LCIR_VAL_VAR: {
    const rv_t* found = env_lookup(ctx ? ctx->env : NULL, v->var);
    if (found)
      return found;
    // No implicit mapping to native functions. Treat as plain symbol.
    return rv_sym(v->var);
  }
  case LCIR_VAL_CONSTR: {
    if (strcmp(v->constr.name, "()") == 0 && v->constr.argc == 0)
      return rv_unit();
    int          n  = v->constr.argc;
    const rv_t** xs = NULL;
    if (n > 0) {
      xs = lsmalloc(sizeof(rv_t*) * n);
      for (int i = 0; i < n; i++)
        xs[i] = eval_value(outfp, v->constr.args[i], ctx);
    }
    return rv_constr(v->constr.name, n, xs);
  }
  case LCIR_VAL_LAM:
    return rv_lam(v->lam.param, v->lam.body, ctx ? ctx->env : NULL);
  case LCIR_VAL_NSLIT: {
    for (int i = 0; i < v->nslit.count; i++) {
      (void)eval_value(outfp, v->nslit.vals[i], ctx);
    }
    return rv_unit();
  }
  }
  return rv_unit();
}

static int truthy(const rv_t* v) {
  if (!v)
    return 0;
  switch (v->kind) {
  case RV_INT:
    return v->ival != 0;
  case RV_SYMBOL:
    return (strcmp(v->sym, "true") == 0) ? 1 : (strcmp(v->sym, "false") == 0 ? 0 : 1);
  case RV_UNIT:
    return 0;
  default:
    return 1;
  }
}

static void print_value(FILE* outfp, const rv_t* v) {
  switch (v->kind) {
  case RV_UNIT:
    fprintf(outfp, "()\n");
    return;
  case RV_INT:
    fprintf(outfp, "%lld\n", v->ival);
    return;
  case RV_STR:
    fprintf(outfp, "%s\n", v->sval);
    return;
  case RV_SYMBOL:
    fprintf(outfp, "%s\n", v->sym);
    return;
  case RV_LAM:
    fprintf(outfp, "<lam>\n");
    return;
  case RV_TOKEN:
    fprintf(outfp, "<token>\n");
    return;
  case RV_NATFUN:
    fprintf(outfp, "<fun %s/%d:%d>\n", v->nf.name, v->nf.arity, v->nf.capc);
    return;
  case RV_CONSTR: {
    if ((strcmp(v->con.name, "[]") == 0 && v->con.argc == 0) ||
        (strcmp(v->con.name, "Nil") == 0 && v->con.argc == 0)) {
      fprintf(outfp, "[]\n");
      return;
    }
    if ((strcmp(v->con.name, ":") == 0 || strcmp(v->con.name, "Cons") == 0) && v->con.argc == 2) {
      const rv_t*  cur = v;
      int          cap = 8, cnt = 0;
      const rv_t** elems = lsmalloc(sizeof(rv_t*) * cap);
      while (cur && cur->kind == RV_CONSTR &&
             (strcmp(cur->con.name, ":") == 0 || strcmp(cur->con.name, "Cons") == 0) &&
             cur->con.argc == 2) {
        if (cnt >= cap) {
          cap *= 2;
          const rv_t** tmp = lsmalloc(sizeof(rv_t*) * cap);
          for (int i = 0; i < cnt; i++)
            tmp[i] = elems[i];
          elems = tmp;
        }
        elems[cnt++] = cur->con.args[0];
        cur          = cur->con.args[1];
      }
      int is_nil = (cur && cur->kind == RV_CONSTR &&
                    ((strcmp(cur->con.name, "[]") == 0 && cur->con.argc == 0) ||
                     (strcmp(cur->con.name, "Nil") == 0 && cur->con.argc == 0)));
      if (is_nil) {
        fprintf(outfp, "[");
        for (int i = 0; i < cnt; i++) {
          if (i)
            fprintf(outfp, ", ");
          const rv_t* e = elems[i];
          switch (e->kind) {
          case RV_INT:
            fprintf(outfp, "%lld", e->ival);
            break;
          case RV_STR:
            fprintf(outfp, "%s", e->sval);
            break;
          case RV_SYMBOL:
            fprintf(outfp, "%s", e->sym);
            break;
          case RV_UNIT:
            fprintf(outfp, "()");
            break;
          default:
            fprintf(outfp, "<v>");
            break;
          }
        }
        fprintf(outfp, "]\n");
        return;
      }
    }
    fprintf(outfp, "(%s", v->con.name);
    for (int i = 0; i < v->con.argc; i++) {
      fprintf(outfp, " ");
      const rv_t* e = v->con.args[i];
      switch (e->kind) {
      case RV_INT:
        fprintf(outfp, "%lld", e->ival);
        break;
      case RV_STR:
        fprintf(outfp, "%s", e->sval);
        break;
      case RV_SYMBOL:
        fprintf(outfp, "%s", e->sym);
        break;
      case RV_UNIT:
        fprintf(outfp, "()");
        break;
      default:
        fprintf(outfp, "<v>");
        break;
      }
    }
    fprintf(outfp, ")\n");
    return;
  case RV_BOTTOM:
    // Keep outward behavior stable for now: print unit on stdout
    fprintf(outfp, "()\n");
    return;
  }
  }
}

static const rv_t* apply_natfun(FILE* outfp, const rv_t* nf, int argc, const rv_t* const* args,
                                eval_ctx_t* ctx) {
  (void)ctx;
  const char* name = nf->nf.name;
  if (argc < nf->nf.arity) {
    int          capc = nf->nf.capc + argc;
    const rv_t** caps = NULL;
    if (capc > 0) {
      caps = lsmalloc(sizeof(rv_t*) * capc);
      for (int i = 0; i < nf->nf.capc; i++)
        caps[i] = nf->nf.caps[i];
      for (int i = 0; i < argc; i++)
        caps[nf->nf.capc + i] = args[i];
    }
    return rv_natfun(name, nf->nf.arity - argc, capc, caps);
  }
  int          tot = nf->nf.capc + argc;
  const rv_t** xs  = (tot > 0) ? lsmalloc(sizeof(rv_t*) * tot) : NULL;
  for (int i = 0; i < nf->nf.capc; i++)
    xs[i] = nf->nf.caps[i];
  for (int i = 0; i < argc; i++)
    xs[nf->nf.capc + i] = args[i];
  // TODO: Native function application requires an explicit registry provided via IR imports.
  //       Current evaluator deliberately avoids any name-based behavior.
  (void)tot; (void)xs; (void)name; // unused until registry is introduced
  return rv_unit();
}

static const rv_t* apply(FILE* outfp, const rv_t* f, int argc, const rv_t* const* args,
                         eval_ctx_t* ctx) {
  if (!f)
    return rv_unit();
  switch (f->kind) {
  case RV_LAM: {
    if (argc != 1)
      return rv_unit();
    env_bind_t* env2 = env_push(f->lam.env, f->lam.param, args[0]);
    eval_ctx_t  sub  = { .env = env2, .has_token = ctx->has_token };
    return eval_expr(outfp, f->lam.body, &sub);
  }
  case RV_NATFUN:
    return apply_natfun(outfp, f, argc, args, ctx);
  default:
    return rv_unit();
  }
}

static const rv_t* eval_expr(FILE* outfp, const lscir_expr_t* e, eval_ctx_t* ctx) {
  if (!e)
    return rv_unit();
  switch (e->kind) {
  case LCIR_EXP_VAL:
    return eval_value(outfp, e->v, ctx);
  case LCIR_EXP_LET: {
    const rv_t* v    = eval_expr(outfp, e->let1.bind, ctx);
    env_bind_t* env2 = env_push(ctx->env, e->let1.var, v);
    eval_ctx_t  sub  = { .env       = env2,
                         .has_token = ctx->has_token ||
                                      (e->let1.bind && e->let1.bind->kind == LCIR_EXP_TOKEN) };
    return eval_expr(outfp, e->let1.body, &sub);
  }
  case LCIR_EXP_APP: {
    const rv_t*  f  = eval_value(outfp, e->app.func, ctx);
    const rv_t** xs = NULL;
    if (e->app.argc > 0) {
      xs = lsmalloc(sizeof(rv_t*) * e->app.argc);
      for (int i = 0; i < e->app.argc; i++)
        xs[i] = eval_value(outfp, e->app.args[i], ctx);
    }
    return apply(outfp, f, e->app.argc, xs, ctx);
  }
  case LCIR_EXP_IF: {
    const rv_t* cv = eval_value(outfp, e->ife.cond, ctx);
    if (truthy(cv))
      return eval_expr(outfp, e->ife.then_e, ctx);
    return eval_expr(outfp, e->ife.else_e, ctx);
  }
  case LCIR_EXP_EFFAPP: {
    const rv_t*  f  = eval_value(outfp, e->effapp.func, ctx);
    const rv_t** xs = NULL;
    if (e->effapp.argc > 0) {
      xs = lsmalloc(sizeof(rv_t*) * e->effapp.argc);
      for (int i = 0; i < e->effapp.argc; i++)
        xs[i] = eval_value(outfp, e->effapp.args[i], ctx);
    }
  // TODO: Effects must be mediated via explicit effectful builtins from an import table.
  //       No implicit symbol-based effects here.
    return rv_unit();
  }
  case LCIR_EXP_TOKEN:
    return rv_tok();
  case LCIR_EXP_MATCH: {
    const rv_t* sv = eval_value(outfp, e->match1.scrut, ctx);
    for (int i = 0; i < e->match1.casec; i++) {
      const lscir_case_t* c = &e->match1.cases[i];
      // Reuse clean matcher logic: only handle trivial equality/constructors via rv_t structure
      // For runtime path, we don’t have thunk infra here; pattern eval lives in clean evaluator only.
      // So we limit to wildcard and variables; others require lowering.
      if (!c->pat || c->pat->kind == LCIR_PAT_WILDCARD || c->pat->kind == LCIR_PAT_VAR) {
        eval_ctx_t sub = { .env = ctx->env, .has_token = ctx->has_token };
        // Bind var is ignored in runtime path until full env wiring exists.
        return eval_expr(outfp, c->body, &sub);
      }
    }
  // No match -> emit a Bottom-style diagnostic and return an internal bottom carrying scrutinee
  fprintf(stderr, "E: <bottom msg=\"match: no case\" at <unknown>:1.1: >\n");
  const rv_t* rels[1] = { sv };
  return rv_bottom("match: no case", 1, rels);
  }
  }
  return rv_unit();
}

int lscir_eval(FILE* outfp, const lscir_prog_t* cir) {
  if (!cir || !cir->root) {
    fprintf(outfp, "()\n");
    return 0;
  }
  eval_ctx_t  ctx = { .env = NULL, .has_token = 0 };
  const rv_t* v   = eval_expr(outfp, cir->root, &ctx);
  print_value(outfp, v);
  return 0;
}

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
  RV_CONSTR
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
    if (strcmp(v->var, "add") == 0)
      return rv_natfun("add", 2, 0, NULL);
    if (strcmp(v->var, "sub") == 0)
      return rv_natfun("sub", 2, 0, NULL);
    if (strcmp(v->var, "chain") == 0)
      return rv_natfun("chain", 2, 0, NULL);
    if (strcmp(v->var, "return") == 0)
      return rv_natfun("return", 1, 0, NULL);
    if (strcmp(v->var, "cons") == 0)
      return rv_natfun("cons", 2, 0, NULL);
    if (strcmp(v->var, "concat") == 0)
      return rv_natfun("concat", 2, 0, NULL);
    if (strcmp(v->var, "is_nil") == 0)
      return rv_natfun("is_nil", 1, 0, NULL);
    if (strcmp(v->var, "head") == 0)
      return rv_natfun("head", 1, 0, NULL);
    if (strcmp(v->var, "tail") == 0)
      return rv_natfun("tail", 1, 0, NULL);
    if (strcmp(v->var, "length") == 0)
      return rv_natfun("length", 1, 0, NULL);
    if (strcmp(v->var, "map") == 0)
      return rv_natfun("map", 2, 0, NULL);
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
  if (strcmp(name, "add") == 0) {
    long long a = xs[0]->ival, b = xs[1]->ival;
    return rv_int(a + b);
  }
  if (strcmp(name, "sub") == 0) {
    long long a = xs[0]->ival, b = xs[1]->ival;
    return rv_int(a - b);
  }
  if (strcmp(name, "return") == 0) {
    return xs[0];
  }
  if (strcmp(name, "chain") == 0) {
    if (tot >= 2) {
      const rv_t* fun = xs[1];
      const rv_t* arg = rv_tok();
      if (fun->kind == RV_LAM) {
        const rv_t* argv[1] = { arg };
        (void)apply(outfp, fun, 1, argv, ctx);
      } else if (fun->kind == RV_NATFUN) {
        const rv_t* argv[1] = { arg };
        (void)apply_natfun(outfp, fun, 1, argv, ctx);
      }
    }
    return rv_unit();
  }
  if (strcmp(name, "cons") == 0) {
    const rv_t** pair = lsmalloc(sizeof(rv_t*) * 2);
    pair[0]           = xs[0];
    pair[1]           = xs[1];
    return rv_constr(":", 2, pair);
  }
  if (strcmp(name, "concat") == 0) {
    // concat: append list xs[0] and list xs[1]; tolerate mixed element types.
    const rv_t* l = xs[0];
    const rv_t* r = xs[1];
    // Collect left spine elements
    const rv_t** acc = NULL;
    int          cap = 8, n = 0;
    acc               = lsmalloc(sizeof(rv_t*) * cap);
    const rv_t* cur   = l;
    while (cur && cur->kind == RV_CONSTR &&
           ((strcmp(cur->con.name, ":") == 0 || strcmp(cur->con.name, "Cons") == 0) && cur->con.argc == 2)) {
      const rv_t* hd = cur->con.args[0];
      const rv_t* tl = cur->con.args[1];
      if (n >= cap) {
        cap *= 2;
        const rv_t** tmp = lsmalloc(sizeof(rv_t*) * cap);
        for (int i = 0; i < n; i++)
          tmp[i] = acc[i];
        acc = tmp;
      }
      acc[n++] = hd;
      cur      = tl;
    }
    // If left is a proper list (ends with Nil/[]), rebuild onto r
    if (cur && cur->kind == RV_CONSTR &&
        ((strcmp(cur->con.name, "[]") == 0 && cur->con.argc == 0) ||
         (strcmp(cur->con.name, "Nil") == 0 && cur->con.argc == 0))) {
      const rv_t* res = r;
      for (int i = n - 1; i >= 0; i--) {
        const rv_t** pair = lsmalloc(sizeof(rv_t*) * 2);
        pair[0]          = acc[i];
        pair[1]          = res;
        res              = rv_constr(":", 2, pair);
      }
      return res;
    }
    // If left isn't a proper list, just return right (conservative)
    return r;
  }
  if (strcmp(name, "is_nil") == 0) {
    const rv_t* v = xs[0];
    int is_nil = (v->kind == RV_CONSTR && ((strcmp(v->con.name, "[]") == 0 && v->con.argc == 0) ||
                                           (strcmp(v->con.name, "Nil") == 0 && v->con.argc == 0)));
    return rv_sym(is_nil ? "true" : "false");
  }
  if (strcmp(name, "head") == 0) {
    const rv_t* v = xs[0];
    if (v->kind == RV_CONSTR &&
        (strcmp(v->con.name, ":") == 0 || strcmp(v->con.name, "Cons") == 0) && v->con.argc == 2)
      return v->con.args[0];
    return rv_unit();
  }
  if (strcmp(name, "tail") == 0) {
    const rv_t* v = xs[0];
    if (v->kind == RV_CONSTR &&
        (strcmp(v->con.name, ":") == 0 || strcmp(v->con.name, "Cons") == 0) && v->con.argc == 2)
      return v->con.args[1];
    return rv_constr("[]", 0, NULL);
  }
  if (strcmp(name, "length") == 0) {
    const rv_t* v   = xs[0];
    int         cnt = 0;
    while (v && v->kind == RV_CONSTR &&
           (strcmp(v->con.name, ":") == 0 || strcmp(v->con.name, "Cons") == 0) &&
           v->con.argc == 2) {
      cnt++;
      v = v->con.args[1];
    }
    if (v && v->kind == RV_CONSTR &&
        ((strcmp(v->con.name, "[]") == 0 && v->con.argc == 0) ||
         (strcmp(v->con.name, "Nil") == 0 && v->con.argc == 0))) {
      return rv_int(cnt);
    }
    return rv_int(0);
  }
  if (strcmp(name, "map") == 0) {
    const rv_t* f   = xs[0];
    const rv_t* lst = xs[1];
    if (!(f->kind == RV_LAM || f->kind == RV_NATFUN))
      return lst;
    const rv_t** acc = NULL;
    int          cap = 8, n = 0;
    acc             = lsmalloc(sizeof(rv_t*) * cap);
    const rv_t* cur = lst;
    while (cur && cur->kind == RV_CONSTR &&
           (strcmp(cur->con.name, ":") == 0 || strcmp(cur->con.name, "Cons") == 0) &&
           cur->con.argc == 2) {
      const rv_t* hd     = cur->con.args[0];
      const rv_t* tl     = cur->con.args[1];
      const rv_t* mapped = hd;
      if (n >= cap) {
        cap *= 2;
        const rv_t** tmp = lsmalloc(sizeof(rv_t*) * cap);
        for (int i = 0; i < n; i++)
          tmp[i] = acc[i];
        acc = tmp;
      }
      acc[n++] = mapped;
      cur      = tl;
    }
    if (cur && cur->kind == RV_CONSTR &&
        ((strcmp(cur->con.name, "[]") == 0 && cur->con.argc == 0) ||
         (strcmp(cur->con.name, "Nil") == 0 && cur->con.argc == 0))) {
      const rv_t* res = rv_constr("[]", 0, NULL);
      for (int i = n - 1; i >= 0; i--) {
        const rv_t** pair = lsmalloc(sizeof(rv_t*) * 2);
        pair[0]           = acc[i];
        pair[1]           = res;
        res               = rv_constr(":", 2, pair);
      }
      return res;
    }
    return lst;
  }
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
    if (f->kind == RV_SYMBOL && strcmp(f->sym, "println") == 0) {
      if (e->effapp.argc == 1 && xs[0]->kind == RV_STR) {
        fprintf(outfp, "%s\n", xs[0]->sval);
        return rv_unit();
      }
    }
    if (f->kind == RV_SYMBOL && strcmp(f->sym, "exit") == 0) {
      return rv_unit();
    }
    return rv_unit();
  }
  case LCIR_EXP_TOKEN:
    return rv_tok();
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

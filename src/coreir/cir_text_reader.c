#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coreir/cir_internal.h"
#include "coreir/coreir.h"
#include "common/malloc.h"

// Very small, permissive s-expr-like reader for the printer output in cir_lower_print.c
// Supports: (int N) (str "...") (var name) (lam name <expr>) (nslit (...)+)
// Expr: (app <val> <val>...) (let name <expr> <expr>) (if <val> <expr> <expr>)
//       (match <val> (<pat> -> <expr>)+)
//       (effapp <val> <val> <val>...) (token)

typedef struct {
  const char* s;
  size_t      n;
  size_t      i;
  FILE*       err;
} rdr_t;

static void skip_ws(rdr_t* r) {
  while (r->i < r->n) {
    if (isspace((unsigned char)r->s[r->i])) {
      r->i++;
      continue;
    }
    if (r->s[r->i] == ';') { // comment to end of line
      while (r->i < r->n && r->s[r->i] != '\n')
        r->i++;
      continue;
    }
    break;
  }
}
static int match(rdr_t* r, char c) {
  skip_ws(r);
  if (r->i < r->n && r->s[r->i] == c) {
    r->i++;
    return 1;
  }
  return 0;
}
static int peek(rdr_t* r, char c) {
  skip_ws(r);
  return (r->i < r->n && r->s[r->i] == c);
}

static char* read_symbol(rdr_t* r) {
  skip_ws(r);
  size_t st = r->i;
  while (r->i < r->n) {
    char ch = r->s[r->i];
    if (isspace((unsigned char)ch) || ch == '(' || ch == ')' || ch == '"')
      break;
    r->i++;
  }
  size_t len = r->i - st;
  if (len == 0)
    return NULL;
  char* out = lsmalloc(len + 1);
  memcpy(out, r->s + st, len);
  out[len] = '\0';
  return out;
}

static char* read_string(rdr_t* r) {
  skip_ws(r);
  if (!match(r, '"'))
    return NULL;
  size_t st = r->i;
  while (r->i < r->n && r->s[r->i] != '"')
    r->i++;
  size_t len = r->i - st;
  char*  out = lsmalloc(len + 1);
  memcpy(out, r->s + st, len);
  out[len] = '\0';
  (void)match(r, '"');
  return out;
}

static long long read_int(rdr_t* r) {
  skip_ws(r);
  int sign = 1;
  if (r->i < r->n && r->s[r->i] == '-') {
    sign = -1;
    r->i++;
  }
  long long v   = 0;
  int       any = 0;
  while (r->i < r->n && isdigit((unsigned char)r->s[r->i])) {
    v = v * 10 + (r->s[r->i] - '0');
    r->i++;
    any = 1;
  }
  if (!any)
    return 0;
  return sign * v;
}

static const lscir_value_t* parse_val(rdr_t* r);
static const lscir_expr_t*  parse_expr(rdr_t* r);
static const lscir_pat_t*   parse_pat(rdr_t* r);
// Parse a value assuming we're already inside an opening '(', i.e., parse the
// value head and its payload and consume the closing ')'.
static const lscir_value_t* parse_val_tail(rdr_t* r);

static const lscir_value_t* mk_int(long long v) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_INT;
  x->ival          = v;
  return x;
}
static const lscir_value_t* mk_str(const char* s) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_STR;
  x->sval          = s;
  return x;
}
static const lscir_value_t* mk_var(const char* s) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_VAR;
  x->var           = s;
  return x;
}
static const lscir_value_t* mk_con(const char* name, int argc, const lscir_value_t* const* args) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_CONSTR;
  x->constr.name   = name;
  x->constr.argc   = argc;
  x->constr.args   = args;
  return x;
}
static const lscir_value_t* mk_lam(const char* p, const lscir_expr_t* b) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_LAM;
  x->lam.param     = p;
  x->lam.body      = b;
  return x;
}
static const lscir_value_t* mk_nslit(int n, const char* const* names,
                                     const lscir_value_t* const* vals) {
  lscir_value_t* x = lsmalloc(sizeof(*x));
  x->kind          = LCIR_VAL_NSLIT;
  x->nslit.count   = n;
  x->nslit.names   = names;
  x->nslit.vals    = vals;
  return x;
}
static const lscir_expr_t* mk_e_val(const lscir_value_t* v) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_VAL;
  e->v            = v;
  return e;
}
static const lscir_expr_t* mk_e_app(const lscir_value_t* f, int argc,
                                    const lscir_value_t* const* args) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_APP;
  e->app.func     = f;
  e->app.argc     = argc;
  e->app.args     = args;
  return e;
}
static const lscir_expr_t* mk_e_let(const char* v, const lscir_expr_t* b, const lscir_expr_t* c) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_LET;
  e->let1.var     = v;
  e->let1.bind    = b;
  e->let1.body    = c;
  return e;
}
static const lscir_expr_t* mk_e_if(const lscir_value_t* c, const lscir_expr_t* t,
                                   const lscir_expr_t* e) {
  lscir_expr_t* x = lsmalloc(sizeof(*x));
  x->kind         = LCIR_EXP_IF;
  x->ife.cond     = c;
  x->ife.then_e   = t;
  x->ife.else_e   = e;
  return x;
}
static const lscir_expr_t* mk_e_eff(const lscir_value_t* f, const lscir_value_t* tok, int argc,
                                    const lscir_value_t* const* args) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_EFFAPP;
  e->effapp.func  = f;
  e->effapp.token = tok;
  e->effapp.argc  = argc;
  e->effapp.args  = args;
  return e;
}
static const lscir_expr_t* mk_e_tok(void) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_TOKEN;
  return e;
}
// --- match helpers ---
static const lscir_pat_t* mk_pat_var(const char* v) {
  lscir_pat_t* p = lsmalloc(sizeof(*p));
  p->kind        = LCIR_PAT_VAR;
  p->var         = v;
  return p;
}
static const lscir_pat_t* mk_pat_wild(void) {
  lscir_pat_t* p = lsmalloc(sizeof(*p));
  p->kind        = LCIR_PAT_WILDCARD;
  return p;
}
static const lscir_pat_t* mk_pat_int(long long x) {
  lscir_pat_t* p = lsmalloc(sizeof(*p));
  p->kind        = LCIR_PAT_INT;
  p->ival        = x;
  return p;
}
static const lscir_pat_t* mk_pat_str(const char* s) {
  lscir_pat_t* p = lsmalloc(sizeof(*p));
  p->kind        = LCIR_PAT_STR;
  p->sval        = s;
  return p;
}
static const lscir_pat_t* mk_pat_con(const char* name, int n, const lscir_pat_t* const* ps) {
  lscir_pat_t* p    = lsmalloc(sizeof(*p));
  p->kind           = LCIR_PAT_CONSTR;
  p->constr.name    = name;
  p->constr.subpats = ps;
  p->constr.subc    = n;
  return p;
}
static const lscir_case_t* mk_case(const lscir_pat_t* pat, const lscir_expr_t* body) {
  lscir_case_t* c = lsmalloc(sizeof(*c));
  c->pat          = pat;
  c->body         = body;
  return c;
}
static const lscir_expr_t* mk_e_match(const lscir_value_t* v, int n,
                                      const lscir_case_t* const* cs) {
  lscir_expr_t* e = lsmalloc(sizeof(*e));
  e->kind         = LCIR_EXP_MATCH;
  e->match1.scrut = v;
  e->match1.cases = (const lscir_case_t*)cs;
  e->match1.casec = n;
  return e;
}

static int accept_kw(rdr_t* r, const char* kw) {
  size_t klen = strlen(kw);
  skip_ws(r);
  if (r->i + klen <= r->n && strncmp(r->s + r->i, kw, klen) == 0) {
    r->i += klen;
    return 1;
  }
  return 0;
}

static const lscir_value_t* parse_val_list(rdr_t* r) {
  if (accept_kw(r, "int")) {
    long long v = read_int(r);
    if (!match(r, ')'))
      return NULL;
    return mk_int(v);
  }
  if (accept_kw(r, "str")) {
    char* s = read_string(r);
    if (!match(r, ')'))
      return NULL;
    return mk_str(s);
  }
  if (accept_kw(r, "var")) {
    char* s = read_symbol(r);
    if (!match(r, ')'))
      return NULL;
    return mk_var(s);
  }
  if (accept_kw(r, "lam")) {
    char*               p = read_symbol(r);
    const lscir_expr_t* b = parse_expr(r);
    if (!match(r, ')'))
      return NULL;
    return mk_lam(p, b);
  }
  if (accept_kw(r, "nslit")) {
    // (nslit (name <val>)+)
    int                   n     = 0;
    const char**          names = NULL;
    const lscir_value_t** vals  = NULL;
    while (peek(r, '(')) {
      match(r, '(');
      char*                nm = read_symbol(r);
      const lscir_value_t* vv = parse_val(r);
      if (!match(r, ')'))
        return NULL;
      const char**          n2 = lsmalloc(sizeof(char*) * (n + 1));
      const lscir_value_t** v2 = lsmalloc(sizeof(lscir_value_t*) * (n + 1));
      for (int i = 0; i < n; i++) {
        n2[i] = names[i];
        v2[i] = vals[i];
      }
      n2[n] = nm;
      v2[n] = vv;
      n++;
      names = n2;
      vals  = v2;
    }
    if (!match(r, ')'))
      return NULL;
    return mk_nslit(n, names, vals);
  }
  // constructor case: (Constr <val>...)
  char* cname = read_symbol(r);
  if (!cname)
    return NULL;
  // parse zero or more values until ')'
  int                   argc = 0;
  const lscir_value_t** args = NULL;
  while (!peek(r, ')')) {
    const lscir_value_t*  av = parse_val(r);
    const lscir_value_t** a2 = lsmalloc(sizeof(lscir_value_t*) * (argc + 1));
    for (int i = 0; i < argc; i++)
      a2[i] = args[i];
    a2[argc] = av;
    argc++;
    args = a2;
  }
  match(r, ')');
  return mk_con(cname, argc, args);
}

static const lscir_value_t* parse_val(rdr_t* r) {
  skip_ws(r);
  if (match(r, '(')) {
    return parse_val_list(r);
  }
  // bare symbol treated as var
  char* s = read_symbol(r);
  if (!s)
    return NULL;
  return mk_var(s);
}

static const lscir_expr_t* parse_expr_list(rdr_t* r) {
  if (accept_kw(r, "app")) {
    const lscir_value_t*  f    = parse_val(r);
    int                   argc = 0;
    const lscir_value_t** args = NULL;
    while (!peek(r, ')')) {
      const lscir_value_t*  av = parse_val(r);
      const lscir_value_t** a2 = lsmalloc(sizeof(lscir_value_t*) * (argc + 1));
      for (int i = 0; i < argc; i++)
        a2[i] = args[i];
      a2[argc] = av;
      argc++;
      args = a2;
    }
    match(r, ')');
    return mk_e_app(f, argc, args);
  }
  if (accept_kw(r, "let")) {
    char*               v = read_symbol(r);
    const lscir_expr_t* b = parse_expr(r);
    const lscir_expr_t* c = parse_expr(r);
    if (!match(r, ')'))
      return NULL;
    return mk_e_let(v, b, c);
  }
  if (accept_kw(r, "if")) {
    const lscir_value_t* cond = parse_val(r);
    const lscir_expr_t*  t    = parse_expr(r);
    const lscir_expr_t*  e    = parse_expr(r);
    if (!match(r, ')'))
      return NULL;
    return mk_e_if(cond, t, e);
  }
  if (accept_kw(r, "match")) {
    const lscir_value_t* scrut = parse_val(r);
    int                  n     = 0;
    const lscir_case_t** cs    = NULL;
    while (peek(r, '(')) {
      match(r, '(');
      const lscir_pat_t* p = parse_pat(r);
      if (!accept_kw(r, "->"))
        return NULL;
      const lscir_expr_t* b = parse_expr(r);
      if (!match(r, ')'))
        return NULL;
      const lscir_case_t** c2 = lsmalloc(sizeof(lscir_case_t*) * (n + 1));
      for (int i = 0; i < n; i++)
        c2[i] = cs[i];
      c2[n] = mk_case(p, b);
      n++;
      cs = c2;
    }
    if (!match(r, ')'))
      return NULL;
    return mk_e_match(scrut, n, cs);
  }
  if (accept_kw(r, "effapp")) {
    const lscir_value_t*  f    = parse_val(r);
    const lscir_value_t*  tok  = parse_val(r);
    int                   argc = 0;
    const lscir_value_t** args = NULL;
    while (!peek(r, ')')) {
      const lscir_value_t*  av = parse_val(r);
      const lscir_value_t** a2 = lsmalloc(sizeof(lscir_value_t*) * (argc + 1));
      for (int i = 0; i < argc; i++)
        a2[i] = args[i];
      a2[argc] = av;
      argc++;
      args = a2;
    }
    match(r, ')');
    return mk_e_eff(f, tok, argc, args);
  }
  if (accept_kw(r, "token")) {
    if (!match(r, ')'))
      return NULL;
    return mk_e_tok();
  }
  // otherwise, try value as expr
  const lscir_value_t* v = parse_val_tail(r);
  if (!v)
    return NULL;
  return mk_e_val(v);
}

static const lscir_expr_t* parse_expr(rdr_t* r) {
  skip_ws(r);
  if (match(r, '('))
    return parse_expr_list(r);
  // allow bare value as expr
  const lscir_value_t* v = parse_val(r);
  return v ? mk_e_val(v) : NULL;
}

// Parse value inside an already-open list: head then payload, and consume ')'
static const lscir_value_t* parse_val_tail(rdr_t* r) {
  if (accept_kw(r, "int")) {
    long long v = read_int(r);
    if (!match(r, ')'))
      return NULL;
    return mk_int(v);
  }
  if (accept_kw(r, "str")) {
    char* s = read_string(r);
    if (!match(r, ')'))
      return NULL;
    return mk_str(s);
  }
  if (accept_kw(r, "var")) {
    char* s = read_symbol(r);
    if (!match(r, ')'))
      return NULL;
    return mk_var(s);
  }
  if (accept_kw(r, "lam")) {
    char*               p = read_symbol(r);
    const lscir_expr_t* b = parse_expr(r);
    if (!match(r, ')'))
      return NULL;
    return mk_lam(p, b);
  }
  if (accept_kw(r, "nslit")) {
    int                   n     = 0;
    const char**          names = NULL;
    const lscir_value_t** vals  = NULL;
    while (peek(r, '(')) {
      match(r, '(');
      char*                nm = read_symbol(r);
      const lscir_value_t* vv = parse_val(r);
      if (!match(r, ')'))
        return NULL;
      const char**          n2 = lsmalloc(sizeof(char*) * (n + 1));
      const lscir_value_t** v2 = lsmalloc(sizeof(lscir_value_t*) * (n + 1));
      for (int i = 0; i < n; i++) {
        n2[i] = names[i];
        v2[i] = vals[i];
      }
      n2[n] = nm;
      v2[n] = vv;
      n++;
      names = n2;
      vals  = v2;
    }
    if (!match(r, ')'))
      return NULL;
    return mk_nslit(n, names, vals);
  }
  // constructor case: (Constr <val>...)
  char* cname = read_symbol(r);
  if (!cname)
    return NULL;
  int                   argc = 0;
  const lscir_value_t** args = NULL;
  while (!peek(r, ')')) {
    const lscir_value_t*  av = parse_val(r);
    const lscir_value_t** a2 = lsmalloc(sizeof(lscir_value_t*) * (argc + 1));
    for (int i = 0; i < argc; i++)
      a2[i] = args[i];
    a2[argc] = av;
    argc++;
    args = a2;
  }
  match(r, ')');
  return mk_con(cname, argc, args);
}

// pattern parsing
static const lscir_pat_t* parse_pat_list(rdr_t* r) {
  if (accept_kw(r, "_")) {
    if (!match(r, ')'))
      return NULL;
    return mk_pat_wild();
  }
  if (accept_kw(r, "var")) {
    char* s = read_symbol(r);
    if (!match(r, ')'))
      return NULL;
    return mk_pat_var(s);
  }
  if (accept_kw(r, "int")) {
    long long v = read_int(r);
    if (!match(r, ')'))
      return NULL;
    return mk_pat_int(v);
  }
  if (accept_kw(r, "str")) {
    char* s = read_string(r);
    if (!match(r, ')'))
      return NULL;
    return mk_pat_str(s);
  }
  // constructor pattern: (Constr <pat>...)
  char* cname = read_symbol(r);
  if (!cname)
    return NULL;
  int                 n  = 0;
  const lscir_pat_t** ps = NULL;
  while (!peek(r, ')')) {
    const lscir_pat_t*  p  = parse_pat(r);
    const lscir_pat_t** p2 = lsmalloc(sizeof(lscir_pat_t*) * (n + 1));
    for (int i = 0; i < n; i++)
      p2[i] = ps[i];
    p2[n] = p;
    n++;
    ps = p2;
  }
  match(r, ')');
  return mk_pat_con(cname, n, ps);
}
static const lscir_pat_t* parse_pat(rdr_t* r) {
  skip_ws(r);
  if (match(r, '('))
    return parse_pat_list(r);
  // bare symbol as var
  char* s = read_symbol(r);
  if (!s)
    return NULL;
  if (strcmp(s, "_") == 0)
    return mk_pat_wild();
  return mk_pat_var(s);
}

const lscir_prog_t* lscir_read_text(FILE* infp, FILE* errfp) {
  // slurp file
  if (!infp)
    return NULL;
  char*  buf = NULL;
  size_t cap = 0;
  size_t len = 0;
  char   tmp[4096];
  while (!feof(infp)) {
    size_t n = fread(tmp, 1, sizeof(tmp), infp);
    if (n == 0)
      break;
    if (len + n + 1 > cap) {
      size_t ncap = cap ? cap * 2 : 8192;
      while (ncap < len + n + 1)
        ncap *= 2;
      char* nb = realloc(buf, ncap);
      if (!nb) {
        free(buf);
        return NULL;
      }
      buf = nb;
      cap = ncap;
    }
    memcpy(buf + len, tmp, n);
    len += n;
    buf[len] = '\0';
  }
  if (!buf)
    return NULL;
  // header check (optional)
  rdr_t rd = { .s = buf, .n = len, .i = 0, .err = errfp };
  skip_ws(&rd);
  if (rd.i < rd.n && rd.s[rd.i] == ';') {
    // read to EOL
    size_t st = rd.i + 1;
    while (rd.i < rd.n && rd.s[rd.i] != '\n')
      rd.i++;
    size_t hlen = rd.i - st;
    while (hlen > 0 && isspace((unsigned char)rd.s[st])) {
      st++;
      hlen--;
    }
    if (hlen >= strlen(LCIR_TEXT_HEADER) &&
        strncmp(rd.s + st, LCIR_TEXT_HEADER, strlen(LCIR_TEXT_HEADER)) != 0) {
      if (errfp)
        fprintf(errfp, "E: lscir: bad text header\n");
      free(buf);
      return NULL;
    }
  }
  const lscir_expr_t* e = parse_expr(&rd);
  if (!e) {
    if (errfp)
      fprintf(errfp, "E: lscir: parse error\n");
    free(buf);
    return NULL;
  }
  lscir_prog_t* cir = lsmalloc(sizeof(*cir));
  cir->ast          = NULL;
  cir->root         = e;
  free(buf);
  return cir;
}

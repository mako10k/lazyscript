%code top {
#include <stdio.h>
#include <stdbool.h>
#include "lazyscript.h"
#include "common/malloc.h"
/* Bison のスタック確保に GC を使う */
#ifndef YYMALLOC
#define YYMALLOC lsmalloc
#endif
#ifndef YYFREE
#define YYFREE lsfree
#endif
}

%code requires {
#include "common/array.h"
#include "common/int.h"
#include "common/io.h"
#include <string.h>
#include "common/loc.h"
#include "common/str.h"
#include "expr/expr.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"
#include "misc/bind.h"
#include "misc/prog.h"
#include "pat/palge.h"
#include "pat/pat.h"
typedef void *yyscan_t;
lsscan_t *yyget_extra(yyscan_t yyscanner);
# define YYLLOC_DEFAULT(Cur, Rhs, N)                      \
do                                                        \
  if (N)                                                  \
    {                                                     \
      (Cur).filename     = YYRHSLOC(Rhs, 1).filename;     \
      (Cur).first_line   = YYRHSLOC(Rhs, 1).first_line;   \
      (Cur).first_column = YYRHSLOC(Rhs, 1).first_column; \
      (Cur).last_line    = YYRHSLOC(Rhs, N).last_line;    \
      (Cur).last_column  = YYRHSLOC(Rhs, N).last_column;  \
    }                                                     \
  else                                                    \
    {                                                     \
      (Cur).filename     = YYRHSLOC(Rhs, 1).filename;     \
      (Cur).first_line   = (Cur).last_line   =            \
        YYRHSLOC(Rhs, 0).last_line;                       \
      (Cur).first_column = (Cur).last_column =            \
        YYRHSLOC(Rhs, 0).last_column;                     \
    }                                                     \
while (0)
}

%union {
    const lsprog_t *prog;
    const lsexpr_t *expr;
    const lsealge_t *ealge;
    const lseappl_t *eappl;
    const lselambda_t *elambda;
    const lsint_t *intval;
    const lsstr_t *strval;
    const lspat_t *pat;
    const lsref_t *ref;
    const lspalge_t *palge;
    const lsbind_t *bind;
    const lsbind_t *bind_ent;
    const lseclosure_t *eclosure;
    const lsarray_t *array;
}

%code {
int yylex(YYSTYPE *yysval, YYLTYPE *yylloc, yyscan_t yyscanner);

// ---- Small helpers to reduce duplication in grammar actions ----
static inline const lsexpr_t *mk_nsref_at(yyscan_t yyscanner, lsloc_t loc) {
  const char *ns = lsscan_get_sugar_ns(yyget_extra(yyscanner));
  return lsexpr_with_loc(lsexpr_new_ref(lsref_new(lsstr_cstr(ns), loc)), loc);
}

// (~ <ns> name)
static inline const lsexpr_t *mk_prelude_func(yyscan_t yyscanner, lsloc_t loc, const char *name) {
  const lsexpr_t *nsref = mk_nsref_at(yyscanner, loc);
  const lsexpr_t *sym = lsexpr_new_alge(lsealge_new(lsstr_cstr(name), 0, NULL));
  const lsexpr_t *args[] = { sym };
  return lsexpr_with_loc(lsexpr_new_appl(lseappl_new(nsref, 1, args)), loc);
}

// forward decl for mk_call1 used below
static inline const lsexpr_t *mk_call1(const lsexpr_t *fn, lsloc_t loc, const lsexpr_t *arg);

// (~ <ns> .name) where .name is a first-class symbol
static inline const lsexpr_t *mk_prelude_func_sym(yyscan_t yyscanner, lsloc_t loc, const char *dotname) {
  const lsexpr_t *nsref = mk_nsref_at(yyscanner, loc);
  const lsexpr_t *sym = lsexpr_with_loc(lsexpr_new_symbol(lsstr_cstr(dotname)), loc);
  return mk_call1(nsref, loc, sym);
}

// Apply any function to a single argument with location
static inline const lsexpr_t *mk_call1(const lsexpr_t *fn, lsloc_t loc, const lsexpr_t *arg) {
  const lsexpr_t *args[] = { arg };
  return lsexpr_with_loc(lsexpr_new_appl(lseappl_new(fn, 1, args)), loc);
}

// (~ <ns> <arg>) — sugar helper when fn is just the namespace ref
static inline const lsexpr_t *mk_prelude_call1(yyscan_t yyscanner, lsloc_t loc, const lsexpr_t *arg) {
  const lsexpr_t *nsref = mk_nsref_at(yyscanner, loc);
  return mk_call1(nsref, loc, arg);
}

static inline int is_dot_symbol_expr(const lsexpr_t *e) {
  if (lsexpr_typeof(e) != LSEQ_SYMBOL) return 0;
  const char *s = lsstr_get_buf(lsexpr_get_symbol(e));
  return (s && s[0] == '.') ? 1 : 0;
}

// Build nested lambdas from an array of patterns ps[0..argc-1] around body
static inline const lsexpr_t *curry_lambdas(const lspat_t *const *ps, lssize_t argc, const lsexpr_t *body) {
  for (lssize_t i = argc; i > 0; --i) {
    body = lsexpr_new_lambda(lselambda_new(ps[i - 1], body));
  }
  return body;
}

// Build list as nested cons for expressions: [e1,...,en] => :(e1, :(e2, ...(:(en, []))))
static inline const lsealge_t *build_ealge_list_from_array(lssize_t argc, const lsexpr_t *const *es) {
  const lsealge_t *chain = lsealge_new(lsstr_cstr("[]"), 0, NULL);
  for (lssize_t i = argc; i > 0; --i) {
    const lsexpr_t *args2[2];
    args2[0] = es[i - 1];
    args2[1] = lsexpr_new_alge(chain);
    chain    = lsealge_new(lsstr_cstr(":"), 2, args2);
  }
  return chain;
}

// Build list as nested cons for patterns
static inline const lspalge_t *build_palge_list_from_array(lssize_t argc, const lspat_t *const *ps) {
  const lspalge_t *chain = lspalge_new(lsstr_cstr("[]"), 0, NULL);
  for (lssize_t i = argc; i > 0; --i) {
    const lspat_t *args2[2];
    args2[0] = ps[i - 1];
    args2[1] = lspat_new_alge(chain);
    chain    = lspalge_new(lsstr_cstr(":"), 2, args2);
  }
  return chain;
}

// Build tuple for expressions: (e1, e2, ..., en) => "," constructor with n args
static inline const lsealge_t *build_ealge_tuple_from_array(lssize_t argc, const lsexpr_t *const *args) {
  return lsealge_new(lsstr_cstr(","), argc, args);
}

// Build tuple for patterns
static inline const lspalge_t *build_palge_tuple_from_array(lssize_t argc, const lspat_t *const *args) {
  return lspalge_new(lsstr_cstr(","), argc, args);
}

// --- Tiny primitives for common constructors ---
static inline const lsealge_t *mk_econs2(const lsexpr_t *a, const lsexpr_t *b) {
  const lsexpr_t *args2[] = { a, b };
  return lsealge_new(lsstr_cstr(":"), 2, args2);
}
static inline const lspalge_t *mk_pcons2(const lspat_t *a, const lspat_t *b) {
  const lspat_t *args2[] = { a, b };
  return lspalge_new(lsstr_cstr(":"), 2, args2);
}
static inline const lsexpr_t *mk_constr0_expr_sym(const lsstr_t *sym) {
  return lsexpr_new_alge(lsealge_new(sym, 0, NULL));
}
static inline const lsexpr_t *mk_member_tag_expr(void) {
  return lsexpr_new_alge(lsealge_new(lsstr_cstr("member"), 0, NULL));
}
static inline const lsexpr_t *mk_unit_expr(void) {
  return lsexpr_new_alge(lsealge_new(lsstr_cstr(","), 0, NULL));
}

// --- Helpers for do-block sugar ---
static inline const lsexpr_t *mk_prelude_return(yyscan_t yyscanner, lsloc_t loc) {
  return mk_prelude_func(yyscanner, loc, "return");
}
static inline const lsexpr_t *mk_prelude_bind(yyscan_t yyscanner, lsloc_t loc) {
  return mk_prelude_func(yyscanner, loc, "bind");
}
static inline const lsexpr_t *mk_prelude_chain(yyscan_t yyscanner, lsloc_t loc) {
  return mk_prelude_func(yyscanner, loc, "chain");
}
// bind A (x -> body)
static inline const lsexpr_t *mk_bind_with_ref_body(yyscan_t yyscanner, lsloc_t loc, const lsexpr_t *xexpr, const lsexpr_t *ae, const lsexpr_t *body) {
  const lsexpr_t *bind = mk_prelude_bind(yyscanner, loc);
  const lsref_t *x = lsexpr_get_ref(xexpr);
  const lspat_t *pat = lspat_new_ref(x);
  const lsexpr_t *lam = lsexpr_new_lambda(lselambda_new(pat, body));
  const lsexpr_t *args2[] = { ae, lam };
  return lsexpr_new_appl(lseappl_new(bind, 2, args2));
}
// chain A (_ -> body)
static inline const lsexpr_t *mk_chain_ignore(yyscan_t yyscanner, lsloc_t loc, const lsexpr_t *ae, const lsexpr_t *body) {
  const lsexpr_t *chain = mk_prelude_chain(yyscanner, loc);
  const lspat_t *argpat = lspat_new_wild();
  const lsexpr_t *lam = lsexpr_new_lambda(lselambda_new(argpat, body));
  const lsexpr_t *args2[] = { ae, lam };
  return lsexpr_new_appl(lseappl_new(chain, 2, args2));
}
// return x
static inline const lsexpr_t *mk_return_x(yyscan_t yyscanner, lsloc_t loc, const lsexpr_t *xexpr) {
  return mk_call1(mk_prelude_return(yyscanner, loc), loc, xexpr);
}
}

%define api.pure full
%define api.location.type {lsloc_t}
%parse-param {yyscan_t yyscanner}
%lex-param {yyscan_t yyscanner}
%header "parser.h"
%output "parser.c"
%locations
%define parse.error verbose
%define parse.lac full

%initial-action {
  yylloc = lsloc(lsscan_get_filename(yyget_extra(yyscanner)), 1, 1, 1, 1);
}

%expect 5

%nterm <prog> prog
%nterm <expr> expr expr1 expr2 expr3 expr4 expr5 efact dostmts
%nterm <expr> lamchain
%nterm <array> dostmt
%nterm <elambda> elambda
%nterm <pat> lamparam lamparam2
%nterm <array> lamparams
%nterm <array> earray parray
%nterm <ealge> ealge elist econs etuple
%nterm <eappl> eappl
%nterm <pat> pat pat1 pat2 pat3
%nterm <ref> pref
%nterm <palge> palge plist pcons ptuple
%nterm <array> bind bind_list
%nterm <bind> bind_single
%nterm <ref> funlhs
%nterm <eclosure> closure
%nterm <array> nslit_entries
%nterm <array> nslit_entry


%token <intval> LSTINT
%token <strval> LSTSYMBOL
%token <strval> LSTDOTSYMBOL
%token <strval> LSTPRELUDE_SYMBOL
%token <strval> LSTENVOP
%token <strval> LSTREFSYM
%token <strval> LSTSTR
%token LSTARROW
%token LSTOROR
%token LSTCARET /* lexer must return this for '^' */
%token LSTCARETBAR /* lexer must return this for '^|' */
%token LSTLEFTARROW
%token LSTWILDCARD
%right '|'
%right ':'
%right LSTOROR LSTCARETBAR
/* removed legacy tokens */

%start prog

%%

prog:
      expr ';' { $$ = lsprog_new($1, lsscan_take_comments(yyget_extra(yyscanner))); lsscan_set_prog(yyget_extra(yyscanner), $$); }
    | expr     { $$ = lsprog_new($1, lsscan_take_comments(yyget_extra(yyscanner))); lsscan_set_prog(yyget_extra(yyscanner), $$); }
    ;

bind:
      bind_list { $$ = $1; }
    ;

funlhs:
      pref { $$ = $1; }
    ;

bind_single:
      funlhs lamparams '=' expr {
        // ~f p1 p2 ... = body  ==>  ~f = \p1 -> \p2 -> ... -> body
        const lspat_t *lhs = lspat_new_ref($1);
        lssize_t argc = lsarray_get_size($2);
        const lspat_t *const *ps = (const lspat_t *const *)lsarray_get($2);
  const lsexpr_t *b = curry_lambdas(ps, argc, $4);
        $$ = lsbind_new(lhs, b);
      }
    | pat '=' expr { $$ = lsbind_new($1, $3); }
    ;

bind_list:
      ';' bind_single { $$ = lsarray_new(1, $2); }
    | bind_list ';' bind_single { $$ = lsarray_push($1, $3); }
    ;

expr:
      expr1 { $$ = $1; }
    ;

expr1:
  expr2 { $$ = $1; }
  ;

expr2:
      expr3 { $$ = $1; }
  | econs { $$ = lsexpr_with_loc(lsexpr_new_alge($1), @$);}    
    ;

econs:
      expr1 ':' expr3 {
  $$ = mk_econs2($1, $3);
      }
    ;

expr3:
    expr4 { $$ = $1; }
  | expr4 LSTOROR expr3 { $$ = lsexpr_with_loc(lsexpr_new_choice(lsechoice_new_kind($1, $3, LSECHOICE_EXPR)), @$); }
  | expr4 LSTCARETBAR lamchain { $$ = lsexpr_with_loc(lsexpr_new_choice(lsechoice_new_kind($1, $3, LSECHOICE_CATCH)), @$); }
    ;

// Lambda-only choice chain appears only at efact level, starting from a lambda.
// Right-associative: \a -> A | \b -> B | \c -> C  ==  \a -> A | (\b -> B | (\c -> C))
lamchain:
      elambda { $$ = lsexpr_with_loc(lsexpr_new_lambda($1), @$); }
    | elambda '|' lamchain {
        const lsexpr_t *lhs = lsexpr_with_loc(lsexpr_new_lambda($1), @1);
        $$ = lsexpr_with_loc(lsexpr_new_choice(lsechoice_new_kind(lhs, $3, LSECHOICE_LAMBDA)), @$);
      }
    ;

eappl:
      efact expr5 {
        // 関数位置がリテラル（.sym/Int/Str）の適用は禁止
        int head_is_literal = 0;
        switch (lsexpr_typeof($1)) {
          case LSEQ_INT:
          case LSEQ_STR:
            head_is_literal = 1; break;
          case LSEQ_SYMBOL:
            head_is_literal = 1; break;
          default: break;
        }
        if (head_is_literal) {
          yyerror(&@1, yyget_extra(yyscanner), "literal (symbol/int/str) cannot be applied");
          YYERROR;
        }
        $$ = lseappl_new($1, 1, &$2);
      }
    | eappl expr5 {
        // 直前引数と今回引数が連続して .sym かつ、すでに非 .sym 引数が存在する場合はエラー
        // 目的: `... X .f .g` のような外側アプリケーションでの隣接 .sym を禁止
        // ただし `~B .c .d` のような内側の純 .sym チェーンは許可する
        int is_dot = is_dot_symbol_expr($2);
        int last_is_dot = 0, has_non_dot_arg = 0;
        lssize_t argc = lseappl_get_argc($1);
        if (argc > 0) {
          // 直前引数を確認
          const lsexpr_t *last = lseappl_get_args($1)[argc - 1];
          last_is_dot = is_dot_symbol_expr(last);
          // 既存引数に非 .sym が含まれるか
          for (lssize_t i = 0; i < argc; i++) {
            const lsexpr_t *ai = lseappl_get_args($1)[i];
            if (!is_dot_symbol_expr(ai)) {
              has_non_dot_arg = 1; break;
            }
          }
        }
        if (is_dot && last_is_dot && has_non_dot_arg) {
          // ただし直前の直前が参照(~X)なら、`~X .c .d` の継続として許可
          int prevprev_is_ref = 0;
          if (argc >= 2) {
            const lsexpr_t *pp = lseappl_get_args($1)[argc - 2];
            if (lsexpr_typeof(pp) == LSEQ_REF) prevprev_is_ref = 1;
          }
          if (!prevprev_is_ref) {
          yyerror(&@2, yyget_extra(yyscanner), "consecutive dot-symbol arguments are not allowed");
          YYERROR;
          }
        }
        $$ = lseappl_add_arg($1, $2);
      }
    ;

expr4:
      expr5 { $$ = $1; }
  | eappl { $$ = lsexpr_with_loc(lsexpr_new_appl($1), @$); }
  | ealge { $$ = lsexpr_with_loc(lsexpr_new_alge($1), @$); }
  | lamchain { $$ = $1; }
  /* Grouped lambda-choice: allow ( \x -> a | \y -> b ) as a single expression */
  | '(' lamchain ')' { $$ = $2; }
    ;

ealge:
      LSTSYMBOL expr5 { $$ = lsealge_new($1, 1, &$2); }
    | ealge expr5 { $$ = lsealge_add_arg($1, $2); }
    ;

expr5:
      efact { $$ = $1; }
  | LSTSYMBOL { $$ = lsexpr_with_loc(mk_constr0_expr_sym($1), @$); }
  | LSTDOTSYMBOL { $$ = lsexpr_with_loc(lsexpr_new_symbol($1), @$); }
    
    ;

efact:
  LSTINT { $$ = lsexpr_with_loc(lsexpr_new_int($1), @$); }
  | LSTSTR { $$ = lsexpr_with_loc(lsexpr_new_str($1), @$); }
  | LSTREFSYM { $$ = lsexpr_with_loc(lsexpr_new_ref(lsref_new($1, @$)), @$); }
  | LSTPRELUDE_SYMBOL {
    // ~~symbol => (~<ns> symbol) where symbol is a plain name (no leading dot)
    const lsexpr_t *sym = lsexpr_with_loc(lsexpr_new_alge(lsealge_new($1, 0, NULL)), @$);
    $$ = mk_prelude_call1(yyscanner, @$, sym);
      }
    | etuple {
        lssize_t argc = lsealge_get_argc($1);
        const lsexpr_t *const *args = lsealge_get_args($1);
  $$ = argc == 1 ? args[0] : lsexpr_with_loc(lsexpr_new_alge($1), @$);
    }
  | elist { $$ = lsexpr_with_loc(lsexpr_new_alge($1), @$); }
  | closure { $$ = lsexpr_with_loc(lsexpr_new_closure($1), @$); }
  | LSTCARET '(' expr ')' {
        // ^(Expr): dedicated AST node; runtime constructs Bottom from the value of Expr
        $$ = lsexpr_with_loc(lsexpr_new_raise($3), @$);
      }
  | LSTENVOP {
  // !ident => (~<ns> .env .ident)
  // CoreIR 降ろしは (~prelude .env .println) も認識する（cir_lower_print.c 側で対応）
  const char* id = lsstr_get_buf($1);
  // Build (~<ns> .env)
  const lsexpr_t *env = mk_prelude_func_sym(yyscanner, @$, ".env");
  // Build dot symbol .ident
  char* buf = NULL; size_t bl = 0; if (id) { bl = strlen(id) + 2; buf = lsmalloc(bl); buf[0] = '.'; memcpy(buf + 1, id, strlen(id) + 1); }
  const lsexpr_t *sym = lsexpr_with_loc(lsexpr_new_symbol(lsstr_new(buf ? buf : ".", (lssize_t)(buf ? (bl - 1) : 1))), @$);
  if (buf) lsfree(buf);
  $$ = mk_call1(env, @$, sym);
      }
  | '!' '{' dostmts '}' { $$ = $3; }
    | '{' nslit_entries '}' {
          // Build AST-level pure namespace literal ({ ... })
          lssize_t ec = $2 ? lsarray_get_size($2) : 0;
          const lsstr_t** names = ec ? lsmalloc(sizeof(lsstr_t*) * ec) : NULL;
          const lsexpr_t** exprs = ec ? lsmalloc(sizeof(lsexpr_t*) * ec) : NULL;
          for (lssize_t i = 0; i < ec; i++) {
            const lsarray_t *ent = (const lsarray_t*)lsarray_get($2)[i];
            const lsexpr_t *sym = (const lsexpr_t*)lsarray_get(ent)[1];
            const lsstr_t *sname = NULL;
            if (lsexpr_typeof(sym) == LSEQ_SYMBOL) sname = lsexpr_get_symbol(sym);
            else if (lsexpr_typeof(sym) == LSEQ_ALGE && lsealge_get_argc(lsexpr_get_alge(sym)) == 0)
              sname = lsealge_get_constr(lsexpr_get_alge(sym));
            names[i] = sname;
            exprs[i] = (const lsexpr_t*)lsarray_get(ent)[2];
          }
          const lsenslit_t *ns = lsenslit_new(ec, names, exprs);
          $$ = lsexpr_with_loc(lsexpr_new_nslit(ns), @$);
        }
    ;

// Pure nslit entries: members only (locals within ns literal are deprecated)
nslit_entries:
  /* empty */ { $$ = NULL; }
  | nslit_entry { $$ = lsarray_new(1, $1); }
  | nslit_entries ';' nslit_entry { $$ = lsarray_push($1, $3); }
  | nslit_entries ';' { $$ = $1; }
  ;

nslit_entry:
  // member: .symbol '=' expr
  LSTDOTSYMBOL '=' expr {
  const lsexpr_t *tag = mk_member_tag_expr();
    const lsexpr_t *sym = lsexpr_with_loc(lsexpr_new_symbol($1), @1);
    $$ = lsarray_new(3, tag, sym, $3);
  }
  | LSTDOTSYMBOL lamparams '=' expr {
    // .sym p1 p2 ... = body  ==>  .sym = \p1 -> \p2 -> ... -> body
  const lsexpr_t *tag = mk_member_tag_expr();
    const lsexpr_t *sym = lsexpr_with_loc(lsexpr_new_symbol($1), @1);
    lssize_t argc = lsarray_get_size($2);
    const lspat_t *const *ps = (const lspat_t *const *)lsarray_get($2);
  const lsexpr_t *b = curry_lambdas(ps, argc, $4);
    $$ = lsarray_new(3, tag, sym, b);
  }
  ;

// do-block style sugar inside braces
// Encoding for intermediate stmt node (dostmt: <array>):
//   - Plain statement: [ expr ]
//   - Bind statement:  [ refexpr, expr ]  where refexpr is (~ x)

dostmts:
      /* empty */ {
        // Empty do-block yields unit: return ()
        $$ = mk_call1(mk_prelude_return(yyscanner, @$), @$, mk_unit_expr());
      }
  | dostmt {
        lssize_t sz = lsarray_get_size($1);
        if (sz == 1) {
          // Final single statement: evaluate in effects and return its value
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[0];
          // bind A (x -> return x)
          const lsref_t *xr = lsref_new(lsstr_cstr("x"), @$);
          const lsexpr_t *xexpr = lsexpr_new_ref(xr);
          const lsexpr_t *ret_x = mk_return_x(yyscanner, @$, xexpr);
          $$ = mk_bind_with_ref_body(yyscanner, @$, xexpr, ae, ret_x);
        } else {
          // Final tail bind: desugar to bind A (x -> return x)
          const lsexpr_t *xexpr = (const lsexpr_t *)lsarray_get($1)[0];
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[1];
          const lsexpr_t *ret_x = mk_return_x(yyscanner, @$, xexpr);
          $$ = mk_bind_with_ref_body(yyscanner, @$, xexpr, ae, ret_x);
        }
      }
    | dostmt ';' dostmts {
        lssize_t sz = lsarray_get_size($1);
        if (sz == 1) {
          // chain A (_ -> rest)
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[0];
          $$ = mk_chain_ignore(yyscanner, @$, ae, $3);
        } else {
          // bind A (x -> rest)
          const lsexpr_t *xexpr = (const lsexpr_t *)lsarray_get($1)[0];
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[1];
          $$ = mk_bind_with_ref_body(yyscanner, @$, xexpr, ae, $3);
        }
      }
    ;



dostmt:
      pref LSTLEFTARROW expr { const lsexpr_t *re = lsexpr_new_ref($1); $$ = lsarray_new(2, re, $3); }
    | LSTSYMBOL LSTLEFTARROW expr {
        const lsref_t *r = lsref_new($1, @1);
        const lsexpr_t *re = lsexpr_new_ref(r);
        $$ = lsarray_new(2, re, $3);
      }
    | expr { $$ = lsarray_new(1, $1); }
    ;

closure:
      '(' expr bind ')' {
          lssize_t bindc = lsarray_get_size($3);
          const lsbind_t *const *binds = (const lsbind_t *const *)lsarray_get($3);
          $$ = lseclosure_new($2, bindc, binds); }
    ;


etuple:
      '(' ')' { $$ = lsealge_new(lsstr_cstr(","), 0, NULL); }
    | '(' earray ')' {
  lssize_t argc = lsarray_get_size($2);
  const lsexpr_t *const *args = (const lsexpr_t *const *)lsarray_get($2);
  $$ = build_ealge_tuple_from_array(argc, args); }
    ;

earray:
      expr { $$ = lsarray_new(1, $1); }
    | earray ',' expr { $$ = lsarray_push($1, $3); }
    ;

elist:
      '[' ']' { $$ = lsealge_new(lsstr_cstr("[]"), 0, NULL); }
    | '[' earray ']' {
  lssize_t argc = lsarray_get_size($2);
  const lsexpr_t *const *es = (const lsexpr_t *const *)lsarray_get($2);
  $$ = build_ealge_list_from_array(argc, es); }
    ;

elambda:
  '\\' lamparams LSTARROW expr3 {
        // \\p1 p2 ... -> body  ==>  \\p1 -> (\\p2 -> ... -> body)
        lssize_t argc = lsarray_get_size($2);
        const lspat_t *const *ps = (const lspat_t *const *)lsarray_get($2);
  const lsexpr_t *b = curry_lambdas(ps + 1, argc - 1, $4);
        $$ = lselambda_new(ps[0], b);
      }
    ;

// Lambda parameter parsing:
//  - A bare constructor token (LSTSYMBOL) is treated as zero-arity pattern in parameter position.
//  - Algebraic patterns with arguments must be parenthesized to be a single parameter.
lamparam:
      lamparam2 { $$ = $1; }
    | lamparam2 '|' lamparam { $$ = lspat_new_or($1, $3); }
    ;

lamparam2:
      LSTSYMBOL { $$ = lspat_new_alge(lspalge_new($1, 0, NULL)); }
    | pat3 { $$ = $1; }
    | '(' pat ')' { $$ = $2; }
    ;

lamparams:
      lamparam { $$ = lsarray_new(1, $1); }
    | lamparams lamparam { $$ = lsarray_push($1, $2); }
    ;

pat:
      pat1 { $$ = $1; }
    | pcons { $$ = lspat_new_alge($1); }
    ;

pcons:
      pat ':' pat {
  $$ = mk_pcons2($1, $3); }
    ;

pat1:
      pat2
    | pat2 '|' pat1 { $$ = lspat_new_or($1, $3); }
    ;

pat2:
      pat3
    | palge { $$ = lspat_new_alge($1); }
    ;

palge:
      LSTSYMBOL { $$ = lspalge_new($1, 0, NULL); }
    | palge pat3 { $$ = lspalge_add_arg($1, $2); }
    ;

pat3:
      ptuple { $$ = lspat_new_alge($1); }
    | plist { $$ = lspat_new_alge($1); }
    | LSTINT { $$ = lspat_new_int($1); }
    | LSTSTR { $$ = lspat_new_str($1); }
    | pref { $$ = lspat_new_ref($1); }
  | LSTWILDCARD { $$ = lspat_new_wild(); }
    | pref '@' pat3 { $$ = lspat_new_as(lspas_new($1, $3)); }
  | LSTCARET '(' pat ')' { $$ = lspat_new_caret($3); }
    ;

pref:
  LSTREFSYM { $$ = lsref_new($1, @$); }
    ;

ptuple:
      '(' ')' { $$ = lspalge_new(lsstr_cstr(","), 0, NULL); }
    | '(' parray ')' {
  lssize_t argc = lsarray_get_size($2);
  const lspat_t *const *args = (const lspat_t *const *)lsarray_get($2);
  $$ = build_palge_tuple_from_array(argc, args); }
    ;

parray:
      pat { $$ = lsarray_new(1, $1); }
    | parray ',' pat { $$ = lsarray_push($1, $3); }
    ;

plist:
      '[' ']' { $$ = lspalge_new(lsstr_cstr("[]"), 0, NULL); }
    | '[' parray ']' {
  lssize_t argc = lsarray_get_size($2);
  const lspat_t *const *ps = (const lspat_t *const *)lsarray_get($2);
  $$ = build_palge_list_from_array(argc, ps); }
    ;

%%

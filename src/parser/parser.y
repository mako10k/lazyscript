%code top {
#include <stdio.h>
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

%expect 36

%nterm <prog> prog
%nterm <expr> expr expr1 expr2 expr3 expr4 expr5 efact dostmts
%nterm <array> dostmt
%nterm <elambda> elambda
%nterm <array> earray parray
%nterm <ealge> ealge elist econs etuple
%nterm <eappl> eappl
%nterm <pat> pat pat1 pat2 pat3
%nterm <ref> pref
%nterm <palge> palge plist pcons ptuple
%nterm <array> bind bind_list
%nterm <bind> bind_single
%nterm <eclosure> closure


%token <intval> LSTINT
%token <strval> LSTSYMBOL
%token <strval> LSTPRELUDESYM
%token <strval> LSTSTR
%token LSTARROW
%token LSTLEFTARROW
%token LSTWILDCARD
%right '|'
%right ':'

%start prog

%%

prog:
      expr ';' { $$ = lsprog_new($1); lsscan_set_prog(yyget_extra(yyscanner), $$); }
    ;

bind:
      bind_list { $$ = $1; }
    ;

bind_single:
      pat '=' expr { $$ = lsbind_new($1, $3); }
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
    | expr2 '|' expr1 { $$ = lsexpr_new_choice(lsechoice_new($1, $3)); }
    ;

expr2:
      expr3 { $$ = $1; }
    | econs { $$ = lsexpr_new_alge($1);}
    ;

econs:
      expr1 ':' expr3 {
        const lsexpr_t *args[] = { $1, $3 };
        $$ = lsealge_new(lsstr_cstr(":"), 2, args);
      }
    ;

expr3:
      expr4 { $$ = $1; }
    | eappl { $$ = lsexpr_new_appl($1); }
    ;

eappl:
      efact expr5 { $$ = lseappl_new($1, 1, &$2); }
    | eappl expr5 { $$ = lseappl_add_arg($1, $2); }
    ;

expr4:
      expr5 { $$ = $1; }
    | ealge { $$ = lsexpr_new_alge($1); }
    ;

ealge:
      LSTSYMBOL expr5 { $$ = lsealge_new($1, 1, &$2); }
    | ealge expr5 { $$ = lsealge_add_arg($1, $2); }
    ;

expr5:
      efact { $$ = $1; }
    | LSTSYMBOL { $$ = lsexpr_new_alge(lsealge_new($1, 0, NULL)); }
    ;

efact:
      LSTINT { $$ = lsexpr_new_int($1); }
    | LSTSTR { $$ = lsexpr_new_str($1); }
    | LSTPRELUDESYM {
        // desugar: ~~sym  ==>  (~<ns> sym), where ns = lsscan_get_sugar_ns(yyextra)
        const char *ns = lsscan_get_sugar_ns(yyget_extra(yyscanner));
        const lsexpr_t *nsref = lsexpr_new_ref(lsref_new(lsstr_cstr(ns), @$));
        const lsexpr_t *sym = lsexpr_new_alge(lsealge_new($1, 0, NULL));
        const lsexpr_t *args[] = { sym };
        $$ = lsexpr_new_appl(lseappl_new(nsref, 1, args));
      }
    | etuple {
        lssize_t argc = lsealge_get_argc($1);
        const lsexpr_t *const *args = lsealge_get_args($1);
        $$ = argc == 1 ? args[0] : lsexpr_new_alge($1);
    }
    | elist { $$ = lsexpr_new_alge($1); }
    | closure { $$ = lsexpr_new_closure($1); }
    | '~' LSTSYMBOL { $$ = lsexpr_new_ref(lsref_new($2, @$)); }
    | elambda { $$ = lsexpr_new_lambda($1); }
    | '{' dostmts '}' { $$ = $2; }
    ;

// do-block style sugar inside braces
// Encoding for intermediate stmt node (dostmt: <array>):
//   - Plain statement: [ expr ]
//   - Bind statement:  [ refexpr, expr ]  where refexpr is (~ x)

dostmts:
      dostmt {
        const char *ns = lsscan_get_sugar_ns(yyget_extra(yyscanner));
        const lsexpr_t *nsref = lsexpr_new_ref(lsref_new(lsstr_cstr(ns), @$));
        const lsexpr_t *ret_sym = lsexpr_new_alge(lsealge_new(lsstr_cstr("return"), 0, NULL));
        const lsexpr_t *ret = lsexpr_new_appl(lseappl_new(nsref, 1, &ret_sym));
        lssize_t sz = lsarray_get_size($1);
        if (sz == 1) {
          const lsexpr_t *e = (const lsexpr_t *)lsarray_get($1)[0];
          const lsexpr_t *args[] = { e };
          $$ = lsexpr_new_appl(lseappl_new(ret, 1, args));
        } else {
          // tail bind: desugar to bind A (x -> return ())
          const lsexpr_t *xexpr = (const lsexpr_t *)lsarray_get($1)[0];
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[1];
          const lsexpr_t *bind_sym = lsexpr_new_alge(lsealge_new(lsstr_cstr("bind"), 0, NULL));
          const lsexpr_t *bind = lsexpr_new_appl(lseappl_new(nsref, 1, &bind_sym));
          const lsref_t *x = lsexpr_get_ref(xexpr);
          const lspat_t *pat = lspat_new_ref(x);
          const lsexpr_t *unit = lsexpr_new_alge(lsealge_new(lsstr_cstr(","), 0, NULL));
          const lsexpr_t *ret_arg[] = { unit };
          const lsexpr_t *ret_unit = lsexpr_new_appl(lseappl_new(ret, 1, ret_arg));
          const lsexpr_t *lam = lsexpr_new_lambda(lselambda_new(pat, ret_unit));
          const lsexpr_t *args2[] = { ae, lam };
          $$ = lsexpr_new_appl(lseappl_new(bind, 2, args2));
        }
      }
    | dostmt ';' dostmts {
        const char *ns = lsscan_get_sugar_ns(yyget_extra(yyscanner));
        const lsexpr_t *nsref = lsexpr_new_ref(lsref_new(lsstr_cstr(ns), @$));
        lssize_t sz = lsarray_get_size($1);
        if (sz == 1) {
          // chain A (_ -> rest)
          const lsexpr_t *chain_sym = lsexpr_new_alge(lsealge_new(lsstr_cstr("chain"), 0, NULL));
          const lsexpr_t *chain = lsexpr_new_appl(lseappl_new(nsref, 1, &chain_sym));
          const lspat_t *argpat = lspat_new_wild();
          const lsexpr_t *lam = lsexpr_new_lambda(lselambda_new(argpat, $3));
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[0];
          const lsexpr_t *args2[] = { ae, lam };
          $$ = lsexpr_new_appl(lseappl_new(chain, 2, args2));
        } else {
          // bind A (x -> rest)
          const lsexpr_t *bind_sym = lsexpr_new_alge(lsealge_new(lsstr_cstr("bind"), 0, NULL));
          const lsexpr_t *bind = lsexpr_new_appl(lseappl_new(nsref, 1, &bind_sym));
          const lsexpr_t *xexpr = (const lsexpr_t *)lsarray_get($1)[0];
          const lsexpr_t *ae = (const lsexpr_t *)lsarray_get($1)[1];
          const lsref_t *x = lsexpr_get_ref(xexpr);
          const lspat_t *pat = lspat_new_ref(x);
          const lsexpr_t *lam = lsexpr_new_lambda(lselambda_new(pat, $3));
          const lsexpr_t *args2[] = { ae, lam };
          $$ = lsexpr_new_appl(lseappl_new(bind, 2, args2));
        }
      }
    ;

dostmt:
      expr { $$ = lsarray_new(1, $1); }
  | pref LSTLEFTARROW expr { const lsexpr_t *re = lsexpr_new_ref($1); $$ = lsarray_new(2, re, $3); }
    | LSTSYMBOL LSTLEFTARROW expr {
        const lsref_t *r = lsref_new($1, @1);
        const lsexpr_t *re = lsexpr_new_ref(r);
        $$ = lsarray_new(2, re, $3);
      }
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
        $$ = lsealge_new(lsstr_cstr(","), argc, args); }
    ;

earray:
      expr { $$ = lsarray_new(1, $1); }
    | earray ',' expr { $$ = lsarray_push($1, $3); }
    ;

elist:
      '[' ']' { $$ = lsealge_new(lsstr_cstr("[]"), 0, NULL); }
    | '[' earray ']' {
        // Desugar [e1, e2, ..., en] into :(e1, :(e2, ...(:(en, [])))
        lssize_t argc = lsarray_get_size($2);
        const lsexpr_t *const *es = (const lsexpr_t *const *)lsarray_get($2);
        const lsealge_t *chain = lsealge_new(lsstr_cstr("[]"), 0, NULL);
        for (lssize_t i = argc; i > 0; i--) {
          const lsexpr_t *args2[2];
          args2[0] = es[i - 1];
          args2[1] = lsexpr_new_alge(chain);
          chain    = lsealge_new(lsstr_cstr(":"), 2, args2);
        }
        $$ = chain; }
    ;

elambda:
      '\\' pat LSTARROW expr3 { $$ = lselambda_new($2, $4); }
    ;

pat:
      pat1 { $$ = $1; }
    | pcons { $$ = lspat_new_alge($1); }
    ;

pcons:
      pat ':' pat {
        const lspat_t *args[] = { $1, $3 };
        $$ = lspalge_new(lsstr_cstr(":"), 2, args); }
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
    ;

pref:
      '~' LSTSYMBOL { $$ = lsref_new($2, @$); }
    ;

ptuple:
      '(' ')' { $$ = lspalge_new(lsstr_cstr(","), 0, NULL); }
    | '(' parray ')' {
      lssize_t argc = lsarray_get_size($2);
      const lspat_t *const *args = (const lspat_t *const *)lsarray_get($2);
      $$ = lspalge_new(lsstr_cstr(","), argc, args); }
    ;

parray:
      pat { $$ = lsarray_new(1, $1); }
    | parray ',' pat { $$ = lsarray_push($1, $3); }
    ;

plist:
      '[' ']' { $$ = lspalge_new(lsstr_cstr("[]"), 0, NULL); }
    | '[' parray ']' {
        // Desugar [p1, p2, ..., pn] into :(p1, :(p2, ...(:(pn, [])))
        lssize_t argc = lsarray_get_size($2);
        const lspat_t *const *ps = (const lspat_t *const *)lsarray_get($2);
        const lspalge_t *chain = lspalge_new(lsstr_cstr("[]"), 0, NULL);
        for (lssize_t i = argc; i > 0; i--) {
          const lspat_t *args2[2];
          args2[0] = ps[i - 1];
          args2[1] = lspat_new_alge(chain);
          chain    = lspalge_new(lsstr_cstr(":"), 2, args2);
        }
        $$ = chain; }
    ;

%%

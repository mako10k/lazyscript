%code top {
#include <stdio.h>
#include "lazyscript.h"
}

%code requires {
#include "prog.h"
#include "expr.h"
#include "ealge.h"
#include "appl.h"
#include "int.h"
#include "str.h"
#include "array.h"
#include "pat.h"
#include "palge.h"
typedef void *yyscan_t;
}

%union {
    lsprog_t *lsprog;
    lsexpr_t *lsexpr;
    lsealge_t *lsealge;
    lsappl_t *lsappl;
    lslambda_t *lslambda;
    const lsint_t *lsint;
    const lsstr_t *lsstr;
    lsarray_t *lsarray;
    lspat_t *lspat;
    lspalge_t *lspalge;
}

%code {

}

%define api.pure full
%parse-param {yyscan_t *p}
%lex-param {yyscan_t *p}
%header "parser.h"
%output "parser.c"
%locations

%nterm <lsprog> prog
%nterm <lsexpr> expr expr1 expr2 expr3 expr4 efact
%nterm <lslambda> elambda
%nterm <lsarray> earray parray
%nterm <lsealge> ealge elist econs etuple
%nterm <lsappl> eappl
%nterm <lspat> pat pat1 pat2 pat3
%nterm <lspalge> palge plist pcons ptuple

%token <lsint> LSTINT
%token <lsstr> LSTSYMBOL
%token <lsstr> LSTSTR
%token LSTARROW

%start prog

%%

prog:
      expr { $$ = lsprog($1); }
    ;

expr:
      expr1 { $$ = $1; }
    ;

expr1:
      expr2 { $$ = $1; }
    | econs { $$ = lsexpr_alge($1);}
    | elambda { $$ = lsexpr_lambda($1); }
    ;

econs:
      expr1 ':' expr2 { $$ = lsealge(lsstr_cstr(":")); lsealge_push_arg($$, $1); lsealge_push_arg($$, $3); }
    ;

expr2:
      expr3 { $$ = $1; }
    | eappl { $$ = lsexpr_appl($1); }
    ;

eappl:
      efact expr4 { $$ = lsappl($1); lsappl_push_arg($$, $2); }
    | eappl expr4 { $$ = $1; lsappl_push_arg($$, $2); }
    ;

expr3:
      expr4 { $$ = $1; }
    | ealge { $$ = lsexpr_alge($1); }
    ;

ealge:
      LSTSYMBOL expr4 { $$ = lsealge($1); lsealge_push_arg($$, $2); }
    | ealge expr4 { $$ = $1; lsealge_push_arg($$, $2); }
    ;

expr4:
      efact { $$ = $1; }
    | LSTSYMBOL { $$ = lsexpr_alge(lsealge($1)); }
    ;

efact:
      LSTINT { $$ = lsexpr_int($1); }
    | LSTSTR { $$ = lsexpr_str($1); }
    | etuple { $$ = lsealge_get_argc($1) == 1 ? lsealge_get_arg($1, 0) : lsexpr_alge($1); }
    | elist { $$ = lsexpr_alge($1); }
    ;

etuple:
      '(' ')' { $$ = lsealge(lsstr_cstr(",")); }
    | '(' earray ')' { $$ = lsealge(lsstr_cstr(",")); lsealge_push_args($$, $2); }
    ;

earray:
      expr { $$ = lsarray(1); lsarray_push($$, $1); }
    | earray ',' expr { $$ = $1; lsarray_push($1, $3); }
    ;

elist:
      '[' ']' { $$ = lsealge(lsstr_cstr("[]")); }
    | '[' earray ']' { $$ = lsealge(lsstr_cstr("[]")); lsealge_push_args($$, $2); }
    ;

elambda :
      '\\' pat LSTARROW expr2 { $$ = lslambda($2, $4); }
    ;

pat:
      pat1 { $$ = $1; }
    | pcons { $$ = lspat_alge($1); }
    ;

pcons:
      pat1 ':' pat2 { $$ = lspalge(lsstr_cstr(":")); lspalge_push_arg($$, $1); lspalge_push_arg($$, $3); }
    ;

pat1:
      pat2
    | LSTSYMBOL '@' pat3 { $$ = lspat_as(lsas($1, $3)); }
    ;

pat2:
      pat3
    | palge { $$ = lspat_alge($1); }
    ;

palge:
      LSTSYMBOL { $$ = lspalge($1); }
    | palge pat3 { $$ = $1; lspalge_push_arg($$, $2); }
    ;

pat3:
      ptuple { $$ = lspat_alge($1); }
    | plist { $$ = lspat_alge($1); }
    | LSTINT { $$ = lspat_int($1); }
    | LSTSTR { $$ = lspat_str($1); }
    ;

ptuple:
      '(' ')' { $$ = lspalge(lsstr_cstr(",")); }
    | '(' parray ')' { $$ = lspalge(lsstr_cstr(",")); lspalge_push_args($$, $2); }
    ;

parray:
      pat { $$ = lsarray(1); lsarray_push($$, $1); }
    | parray ',' pat { $$ = $1; lsarray_push($1, $3); }
    ;

plist:
      '[' ']' { $$ = lspalge(lsstr_cstr("[]")); }
    | '[' parray ']' { $$ = lspalge(lsstr_cstr("[]")); lspalge_push_args($$, $2); }
    ;

%%

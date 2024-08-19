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
}

%union {
    lsprog_t *lsprog;
    lsexpr_t *lsexpr;
    lsealge_t *lsealge;
    lsappl_t *lsappl;
    const lsint_t *lsint;
    const lsstr_t *lsstr;
    lsarray_t *lsarray;
}

%code {
int yylex(YYSTYPE *yylval);
void yyerror(const char *s);
}

%define api.pure full
%header "parser.h"
%output "parser.c"

%type <lsprog> prog
%type <lsexpr> expr expr1 expr2 expr3 expr4 efact
%type <lsarray> earray
%type <lsealge> ealge elist econs etuple
%type <lsappl> eappl

%token <lsint> LSTINT
%token <lsstr> LSTSYMBOL

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
    ;

econs:
      expr1 ':' expr2 { $$ = lsealge(lsstr_cstr(":")); lsealge_push_arg($$, $1); lsealge_push_arg($$, $3); }
    ;

expr2:
      expr3 { $$ = $1; }
    | eappl { $$ = lsexpr_appl($1); }
    ;

eappl:
      expr4 { $$ = lsappl($1); }
    | eappl efact { $$ = $1; lsappl_push_arg($$, $2); }
    ;

expr3:
      expr4 { $$ = $1; }
    | ealge { $$ = lsexpr_alge($1); }
    ;

ealge:
      LSTSYMBOL { $$ = lsealge($1); }
    | ealge expr4 { $$ = $1; lsealge_push_arg($$, $2); }
    ;

expr4:
      efact { $$ = $1; }
    ;

efact:
      LSTINT { $$ = lsexpr_int($1); }
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


%%

void yyerror(const char *s) {
    fprintf(stderr, "error: %s\n", s);
}
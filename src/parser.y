%code top {
#include <stdio.h>
#include "lazyscript.h"
}

%code requires {
#include "prog.h"
#include "expr.h"
#include "ealge.h"
#include "eappl.h"
#include "int.h"
#include "io.h"
#include "str.h"
#include "array.h"
#include "pat.h"
#include "palge.h"
#include "bind.h"
#include "eclosure.h"
#include "elambda.h"
#include "loc.h"
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
    lsprog_t *prog;
    lsexpr_t *expr;
    lsealge_t *ealge;
    lseappl_t *eappl;
    lselambda_t *elambda;
    lselambda_ent_t *elambda_ent;
    const lsint_t *intval;
    const lsstr_t *strval;
    lsarray_t *array;
    lspat_t *pat;
    lspref_t *pref;
    lspalge_t *palge;
    lsbind_t *bind;
    lsbind_ent_t *bind_ent;
    lseclosure_t *eclosure;
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
  yylloc = lsloc(yyget_extra(yyscanner)->filename, 1, 1, 1, 1);
}

%expect 34

%nterm <prog> prog
%nterm <expr> expr expr1 expr2 expr3 expr4 efact
%nterm <elambda_ent> elambda_single
%nterm <elambda> elambda elambda_list
%nterm <array> earray parray
%nterm <ealge> ealge elist econs etuple
%nterm <eappl> eappl
%nterm <pat> pat pat1 pat2 pat3
%nterm <pref> pref
%nterm <palge> palge plist pcons ptuple
%nterm <bind> bind bind_list
%nterm <bind_ent> bind_single
%nterm <eclosure> closure


%token <intval> LSTINT
%token <strval> LSTSYMBOL
%token <strval> LSTSTR
%token LSTARROW
%right ':'

%start prog

%%

prog:
      expr ';' { $$ = lsprog($1); yyget_extra(yyscanner)->prog = $$; }
    ;

bind:
      bind_list { $$ = $1; }
    ;

bind_single:
      pat '=' expr { $$ = lsbind_ent($1, $3); }
    ;

bind_list:
      ';' bind_single { $$ = lsbind(); lsbind_push($$, $2); }
    | bind_list ';' bind_single { $$ = $1; lsbind_push($$, $3); }
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
      efact expr4 { $$ = lseappl($1); lseappl_push_arg($$, $2); }
    | eappl expr4 { $$ = $1; lseappl_push_arg($$, $2); }
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
    | closure { $$ = lsexpr_closure($1); }
    | '~' LSTSYMBOL { $$ = lsexpr_ref(lseref($2, @$)); }
    | elambda { $$ = lsexpr_lambda($1); }
    | '{' expr '}' { $$ = $2; }
    ;

closure:
      '{' expr bind '}' { $$ = lseclosure($2, $3); }
    ;


etuple:
      '(' ')' { $$ = lsealge(lsstr_cstr(",")); }
    | '(' earray ')' { $$ = lsealge(lsstr_cstr(",")); lsealge_push_args($$, $2); }
    ;

earray:
      expr { $$ = lsarray(); lsarray_push($$, $1); }
    | earray ',' expr { $$ = $1; lsarray_push($1, $3); }
    ;

elist:
      '[' ']' { $$ = lsealge(lsstr_cstr("[]")); }
    | '[' earray ']' { $$ = lsealge(lsstr_cstr("[]")); lsealge_push_args($$, $2); }
    ;

elambda:
      elambda_list { $$ = $1; }
    ;

elambda_list:
      elambda_single { $$ = lselambda(); lselambda_push($$, $1); }
    | elambda_list '|' elambda_single { $$ = $1; lselambda_push($1, $3); }
    ;

elambda_single:
      '\\' pat LSTARROW expr { $$ = lselambda_ent($2, $4); }
    ;

pat:
      pat1 { $$ = $1; }
    | pcons { $$ = lspat_alge($1); }
    ;

pcons:
      pat ':' pat { $$ = lspalge(lsstr_cstr(":")); lspalge_push_arg($$, $1); lspalge_push_arg($$, $3); }
    ;

pat1:
      pat2
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
    | pref { $$ = lspat_ref($1); }
    | pref '@' pat3 { $$ = lspat_as(lspas($1, $3)); }
    ;

pref:
      '~' LSTSYMBOL { $$ = lspref($2, @$); }
    ;

ptuple:
      '(' ')' { $$ = lspalge(lsstr_cstr(",")); }
    | '(' parray ')' { $$ = lspalge(lsstr_cstr(",")); lspalge_push_args($$, $2); }
    ;

parray:
      pat { $$ = lsarray(); lsarray_push($$, $1); }
    | parray ',' pat { $$ = $1; lsarray_push($1, $3); }
    ;

plist:
      '[' ']' { $$ = lspalge(lsstr_cstr("[]")); }
    | '[' parray ']' { $$ = lspalge(lsstr_cstr("[]")); lspalge_push_args($$, $2); }
    ;

%%

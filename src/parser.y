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
#include "bind.h"
#include "closure.h"
typedef void *yyscan_t;
lsscan_t *yyget_extra(yyscan_t yyscanner);
}

%union {
    lsprog_t *lsprog;
    lsexpr_t *lsexpr;
    lsealge_t *lsealge;
    lsappl_t *lsappl;
    lslambda_t *lslambda;
    lslambda_ent_t *lslambda_ent;
    const lsint_t *lsint;
    const lsstr_t *lsstr;
    lsarray_t *lsarray;
    lspat_t *lspat;
    lspalge_t *lspalge;
    lsbind_t *lsbind;
    lsbind_ent_t *lsbind_ent;
    lsclosure_t *lsclosure;
}

%code {

}

%define api.pure full
%parse-param {yyscan_t *yyscanner}
%lex-param {yyscan_t *yyscanner}
%header "parser.h"
%output "parser.c"
%locations

%expect 34

%nterm <lsprog> prog
%nterm <lsexpr> expr expr1 expr2 expr3 expr4 efact
%nterm <lslambda_ent> elambda_single
%nterm <lslambda> elambda elambda_list
%nterm <lsarray> earray parray
%nterm <lsealge> ealge elist econs etuple
%nterm <lsappl> eappl
%nterm <lspat> pat pat1 pat2 pat3
%nterm <lspalge> palge plist pcons ptuple
%nterm <lsbind> bind bind_list
%nterm <lsbind_ent> bind_single
%nterm <lsclosure> closure


%token <lsint> LSTINT
%token <lsstr> LSTSYMBOL
%token <lsstr> LSTSTR
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
    | closure { $$ = lsexpr_closure($1); }
    | '~' LSTSYMBOL { $$ = lsexpr_ref(lseref($2)); }
    | elambda { $$ = lsexpr_lambda($1); }
    | '{' expr '}' { $$ = $2; }
    ;

closure:
      '{' expr bind '}' { $$ = lsclosure($2, $3); }
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
      elambda_single { $$ = lslambda(); lslambda_push($$, $1); }
    | elambda_list '|' elambda_single { $$ = $1; lslambda_push($1, $3); }
    ;

elambda_single:
      '\\' pat LSTARROW expr { $$ = lslambda_ent($2, $4); }
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
    | '~' LSTSYMBOL { $$ = lspat_ref(lspref($2)); }
    | '~' LSTSYMBOL '@' pat3 { $$ = lspat_as(lsas(lspref($2), $4)); }
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

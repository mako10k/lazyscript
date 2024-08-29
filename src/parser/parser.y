%code top {
#include <stdio.h>
#include "lazyscript.h"
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
#include "expr/elist.h"
#include "misc/bind.h"
#include "misc/prog.h"
#include "pat/palge.h"
#include "pat/pat.h"
#include "pat/plist.h"
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
    lsealge_t *ealge;
    lseappl_t *eappl;
    const lselambda_t *elambda;
    const lsint_t *intval;
    const lsstr_t *strval;
    const lspat_t *pat;
    const lspref_t *pref;
    lspalge_t *palge;
    lsbind_t *bind;
    const lsbind_entry_t *bind_ent;
    const lseclosure_t *eclosure;
    const lselist_t *elist;
    const lsplist_t *plist;
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

%expect 32

%nterm <prog> prog
%nterm <expr> expr expr1 expr2 expr3 expr4 expr5 efact
%nterm <elambda> elambda
%nterm <elist> earray
%nterm <plist> parray
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
      pat '=' expr { $$ = lsbind_entry_new($1, $3); }
    ;

bind_list:
      ';' bind_single { $$ = lsbind_new(); lsbind_push($$, $2); }
    | bind_list ';' bind_single { $$ = $1; lsbind_push($$, $3); }
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
        $$ = lsealge_new(lsstr_cstr(":"));
        lsealge_add_arg($$, $1);
        lsealge_add_arg($$, $3);
      }
    ;

expr3:
      expr4 { $$ = $1; }
    | eappl { $$ = lsexpr_new_appl($1); }
    ;

eappl:
      efact expr5 { $$ = lseappl_new($1); lseappl_add_arg($$, $2); }
    | eappl expr5 { $$ = $1; lseappl_add_arg($$, $2); }
    ;

expr4:
      expr5 { $$ = $1; }
    | ealge { $$ = lsexpr_new_alge($1); }
    ;

ealge:
      LSTSYMBOL expr5 { $$ = lsealge_new($1); lsealge_add_arg($$, $2); }
    | ealge expr5 { $$ = $1; lsealge_add_arg($$, $2); }
    ;

expr5:
      efact { $$ = $1; }
    | LSTSYMBOL { $$ = lsexpr_new_alge(lsealge_new($1)); }
    ;

efact:
      LSTINT { $$ = lsexpr_new_int($1); }
    | LSTSTR { $$ = lsexpr_new_str($1); }
    | etuple { $$ = lsealge_get_arg_count($1) == 1 ? lsealge_get_arg($1, 0) : lsexpr_new_alge($1); }
    | elist { $$ = lsexpr_new_alge($1); }
    | closure { $$ = lsexpr_new_closure($1); }
    | '~' LSTSYMBOL { $$ = lsexpr_new_ref(lseref_new($2, @$)); }
    | elambda { $$ = lsexpr_new_lambda($1); }
    | '{' expr '}' { $$ = $2; }
    ;

closure:
      '{' expr bind '}' { $$ = lseclosure_new($2, $3); }
    ;


etuple:
      '(' ')' { $$ = lsealge_new(lsstr_cstr(",")); }
    | '(' earray ')' { $$ = lsealge_new(lsstr_cstr(",")); lsealge_concat_args($$, $2); }
    ;

earray:
      expr { $$ = lselist_push(NULL, $1); }
    | earray ',' expr { $$ = lselist_push($1, $3); }
    ;

elist:
      '[' ']' { $$ = lsealge_new(lsstr_cstr("[]")); }
    | '[' earray ']' { $$ = lsealge_new(lsstr_cstr("[]")); lsealge_concat_args($$, $2); }
    ;

elambda:
      '\\' pat LSTARROW expr3 { $$ = lselambda_new($2, $4); }
    ;

pat:
      pat1 { $$ = $1; }
    | pcons { $$ = lspat_new_alge($1); }
    ;

pcons:
      pat ':' pat { $$ = lspalge_new(lsstr_cstr(":")); lsexpr_add_args($$, $1); lsexpr_add_args($$, $3); }
    ;

pat1:
      pat2
    ;

pat2:
      pat3
    | palge { $$ = lspat_new_alge($1); }
    ;

palge:
      LSTSYMBOL { $$ = lspalge_new($1); }
    | palge pat3 { $$ = $1; lsexpr_add_args($$, $2); }
    ;

pat3:
      ptuple { $$ = lspat_new_alge($1); }
    | plist { $$ = lspat_new_alge($1); }
    | LSTINT { $$ = lspat_new_int($1); }
    | LSTSTR { $$ = lspat_new_str($1); }
    | pref { $$ = lspat_new_ref($1); }
    | pref '@' pat3 { $$ = lspat_new_as(lspas_new($1, $3)); }
    ;

pref:
      '~' LSTSYMBOL { $$ = lspref_new($2, @$); }
    ;

ptuple:
      '(' ')' { $$ = lspalge_new(lsstr_cstr(",")); }
    | '(' parray ')' { $$ = lspalge_new(lsstr_cstr(",")); lsexpr_concat_args($$, $2); }
    ;

parray:
      pat { $$ = lsplist_push(NULL, $1); }
    | parray ',' pat { $$ = lsplist_push($1, $3); }
    ;

plist:
      '[' ']' { $$ = lspalge_new(lsstr_cstr("[]")); }
    | '[' parray ']' { $$ = lspalge_new(lsstr_cstr("[]")); lsexpr_concat_args($$, $2); }
    ;

%%

%{
#include "lazyscript.h"
#include "parser/parser.h"
#include "common/loc.h"
#include "common/io.h"
#define YY_USER_ACTION lsloc_update(yylloc, yytext, yyleng);
%}
%option noyywrap
%option noinput
%option nounput
%option reentrant
%option bison-bridge
%option bison-locations
%option outfile="lexer.c"
%option header-file="lexer.h"
%option extra-type="lsscan_t *"

%x COMMENT

%%
#.*            { /* ignore comment */ }
\{-[^-]*       { BEGIN(COMMENT); }
<COMMENT>.*-\} { BEGIN(INITIAL); }
<COMMENT>.*    { /* ignore comment */ }
[ \t\n]+    { /* ignore whitespace */ }
[0-9]+      { yylval->intval = lsint_new(atoi(yytext)); return LSTINT; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval->strval = lsstr_cstr(yytext); return LSTSYMBOL; }
\"([^\\\"]|\\.)*\" { yylval->strval = lsstr_parse(yytext + 1, yyleng - 2); return LSTSTR; }
"->"       { return LSTARROW; }
. { return yytext[0]; }
%%
%{
#include "lazyscript.h"
#include "parser.h"
%}
%option noyywrap
%option noinput
%option nounput
%option reentrant bison-bridge

%%
[ \t\n]+    { /* ignore whitespace */ }
[0-9]+      { yylval->lsint = lsint(atoi(yytext)); return LSTINT; }

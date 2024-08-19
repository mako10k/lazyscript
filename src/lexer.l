%{
#include "lazyscript.h"
#include "parser.h"
#define YY_USER_ACTION \
  { \
    int i = 0; \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    while (i < yyleng) { \
      char *c = memchr(yytext + i, '\n', yyleng - i); \
      if (c == NULL) {\
        i = yyleng; \
        break; \
      } \
      yylloc->last_line ++; \
      yylloc->last_column = 1; \
      i = c - yytext + 1; \
    } \
    yylloc->last_column += yyleng - i; \
  }
%}
%option noyywrap
%option yylineno
%option reentrant
%option bison-bridge
%option bison-locations
%option outfile="lexer.c"
%option header-file="lexer.h"
%option extra-type="lsscan_t *"

%%
[ \t\n]+    { /* ignore whitespace */ }
[0-9]+      { yylval->lsint = lsint(atoi(yytext)); return LSTINT; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval->lsstr = lsstr_cstr(yytext); return LSTSYMBOL; }
\"([^\\\"]|\\.)*\" { yylval->lsstr = lsstr_parse(yytext + 1, yyleng - 2); return LSTSTR; }
"->"       { return LSTARROW; }
[\[\]\(\),:\{\}@\\] { return yytext[0]; }

%%
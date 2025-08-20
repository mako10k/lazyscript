%{
#include "lazyscript.h"
#include "parser/parser.h"
#include "common/loc.h"
#include "common/io.h"
#include "common/malloc.h"
/* Flex 生成コード内の malloc/realloc/free を GC ラッパに切り替える */
#define malloc  lsmalloc
#define realloc lsrealloc
#define free    lsfree
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
[_]         { return LSTWILDCARD; }
[.][a-zA-Z_][a-zA-Z0-9_]* { yylval->strval = lsstr_cstr(yytext); return LSTSYMBOL; }
[~]{2}[0-9]+      { yylval->intval = lsint_new(atoi(yytext + 2)); return LSTPRELUDE_INT; }
[~]{2}[.]?[a-zA-Z_][a-zA-Z0-9_]* {
	/* ~~constr or ~~.symbol */
	const char* s = yytext + 2; /* include leading '.' if present */
	if (s[0] == '.') { yylval->strval = lsstr_cstr(s); return LSTPRELUDE_SYMBOL; }
	yylval->strval = lsstr_cstr(s); return LSTPRELUDE_CONSTR;
}
[~]{2}\"([^\\\"]|\\.)*\" { /* ~~"string" */
	yylval->strval = lsstr_parse(yytext + 2, yyleng - 2);
	return LSTPRELUDE_STR;
}
[~][a-zA-Z_][a-zA-Z0-9_]* {
	/* ~varName => LSTREFSYM (exclude dot-prefixed to avoid ambiguity) */
	yylval->strval = lsstr_cstr(yytext + 1);
	return LSTREFSYM;
}
[a-zA-Z_][a-zA-Z0-9_]* { yylval->strval = lsstr_cstr(yytext); return LSTSYMBOL; }
\"([^\\\"]|\\.)*\" { yylval->strval = lsstr_parse(yytext, yyleng); return LSTSTR; }
"<-"       { return LSTLEFTARROW; }
"->"       { return LSTARROW; }
. { return yytext[0]; }
%%
%{
#include "lazyscript.h"
#include "parser/parser.h"
#include "common/loc.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "misc/prog.h"
/* Keep token locations accurate across includes */
#define YY_USER_ACTION do { \
	if (!(yylloc)->filename) (yylloc)->filename = lsscan_get_filename(yyget_extra(yyscanner)); \
	lsloc_update(yylloc, yytext, yyleng); \
} while (0);

/* --- single-quoted identifier parser (supports same escapes as string) --- */
static int lslex_parse_hex(const char* str, int* pi, int slen) {
	int hex = 0;
	int c1  = *pi < slen ? str[(*pi)++] : -1;
	if (c1 == -1) return -1;
	if ('0' <= c1 && c1 <= '9') hex = c1 - '0';
	else if ('a' <= c1 && c1 <= 'f') hex = c1 - 'a' + 10;
	else if ('A' <= c1 && c1 <= 'F') hex = c1 - 'A' + 10;
	else return 'x';
	int c2 = *pi < slen ? str[(*pi)++] : -1;
	if (c2 == -1) return -1;
	if ('0' <= c2 && c2 <= '9') hex = hex * 16 + (c2 - '0');
	else if ('a' <= c2 && c2 <= 'f') hex = hex * 16 + (c2 - 'a' + 10);
	else if ('A' <= c2 && c2 <= 'F') hex = hex * 16 + (c2 - 'A' + 10);
	return hex;
}

static int lslex_parse_oct(const char* str, int* pi, int slen, int first) {
	int c = first - '0'; int cnt = 0;
	while (cnt < 2) {
		int n = *pi < slen ? str[*pi] : -1;
		if (n < '0' || n > '7') break;
		(*pi)++; cnt++; c = c * 8 + (n - '0');
	}
	return c;
}

static int lslex_parse_esc(const char* str, int* pi, int slen) {
	int c = *pi < slen ? str[(*pi)++] : -1;
	if (c == -1) return -1;
	switch (c) {
		case '\\': return '\\';
		case '\'': return '\'';  /* allow \' inside single-quoted */
		case '"': return '"';
		case 'n': case 'N': return '\n';
		case 't': case 'T': return '\t';
		case 'r': case 'R': return '\r';
		case '0': return '\0';
		case 'a': case 'A': return '\a';
		case 'b': case 'B': return '\b';
		case 'f': case 'F': return '\f';
		case 'v': case 'V': return '\v';
		case 'x': case 'X': return lslex_parse_hex(str, pi, slen);
		default:
			if ('0' <= c && c <= '7') return lslex_parse_oct(str, pi, slen, c);
			return c;
	}
}

/* Parse a single-quoted string like '\'abc\'' from buffer [p, p+len). */
static const lsstr_t* lslex_parse_squoted(const char* p, int len) {
	if (!p || len < 2 || p[0] != '\'' || p[len - 1] != '\'') return NULL;
	int i = 1; /* skip opening quote */
	char* out = lsmalloc(16); size_t cap = 16, n = 0;
	while (i < len - 1) { /* stop before the closing quote at len-1 */
		int c = p[i++];
		if (c == '\\') {
			c = lslex_parse_esc(p, &i, len - 1); /* parse escape up to before closing */
			if (c == -1) { lsfree(out); return NULL; }
		}
		if (cap <= n + 1) { cap *= 2; out = lsrealloc(out, cap); }
		out[n++] = (char)c;
	}
	if (cap <= n + 1) { cap *= 2; out = lsrealloc(out, cap); }
	out[n] = '\0';
	const lsstr_t* ret = lsstr_new(out, (lssize_t)n);
	lsfree(out);
	return ret;
}
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
%x INCLUDE

%%
[ \t\n]*\{-[ ]*#include[^\n]*-\} {
	const char* s = yytext;
	const char* q1 = strchr(s, '"');
	const char* q2 = q1 ? strchr(q1 + 1, '"') : NULL;
	if (!q1 || !q2 || q2 <= q1 + 1) {
		yyerror(yylloc, yyget_extra(yyscanner), "error: malformed include directive");
		BEGIN(INITIAL);
	} else {
		char* fname = lsmalloc((size_t)(q2 - q1));
		memcpy(fname, q1 + 1, (size_t)(q2 - q1 - 1));
		fname[q2 - q1 - 1] = '\0';
		if (lsscan_in_include_chain(yyget_extra(yyscanner), fname)) {
			char buf[256];
			snprintf(buf, sizeof(buf), "error: include cycle detected for '%s'", fname);
			yyerror(yylloc, yyget_extra(yyscanner), buf);
			lsfree(fname);
			BEGIN(INITIAL);
		} else {
			FILE* f = fopen(fname, "r");
			if (!f) {
				char buf[256];
				snprintf(buf, sizeof(buf), "error: cannot open include file '%s'", fname);
				yyerror(yylloc, yyget_extra(yyscanner), buf);
				lsfree(fname);
				BEGIN(INITIAL);
			} else {
				YY_BUFFER_STATE buf = yy_create_buffer(f, YY_BUF_SIZE, yyscanner);
				yypush_buffer_state(buf, yyscanner);
				lsscan_push_filename(yyget_extra(yyscanner), fname);
				lsscan_push_file_fp(yyget_extra(yyscanner), f);
				lsscan_push_include_site(yyget_extra(yyscanner), *yylloc);
				BEGIN(INITIAL);
			}
		}
	}
}
"#"[^\n]*          {
	/* line/EOL comment starting with '#' anywhere on the line */
	lsscan_add_comment(yyget_extra(yyscanner), *yylloc, lsstr_new(yytext, yyleng));
}
\{-[^-]*       { /* begin block comment */ BEGIN(COMMENT); lsscan_add_comment(yyget_extra(yyscanner), *yylloc, lsstr_cstr("{-")); }
<COMMENT>.*-\} { /* end block comment */ lsscan_add_comment(yyget_extra(yyscanner), *yylloc, lsstr_cstr("-}")); BEGIN(INITIAL); }
<COMMENT>.*    { /* accumulate block comment lines */ lsscan_add_comment(yyget_extra(yyscanner), *yylloc, lsstr_new(yytext, yyleng)); }
[ \t\n]+    { /* whitespace resets tight-adjacency */ lsscan_note_ws(yyget_extra(yyscanner)); }
[0-9]+      { yylval->intval = lsint_new(atoi(yytext)); return LSTINT; }
[_]         { return LSTWILDCARD; }
[.][\']([^\\\']|\\.)*[\'] {
	/* dot-quoted symbol: .'Sym  => LSTDOTSYMBOL with leading '.' */
	const lsstr_t* inner = lslex_parse_squoted(yytext + 1, yyleng - 1);
	if (!inner) return yytext[0];
	const char* ib = lsstr_get_buf(inner); lssize_t il = lsstr_get_len(inner);
	char* buf = lsmalloc((size_t)il + 2); buf[0] = '.'; for (lssize_t i = 0; i < il; ++i) buf[1 + i] = ib[i];
	yylval->strval = lsstr_new(buf, il + 1);
	lsfree(buf);
	return LSTDOTSYMBOL;
}
[~][\']([^\\\']|\\.)*[\'] {
	/* tilde-quoted ref: ~'Var => LSTREFSYM without leading ~ */
	const lsstr_t* inner = lslex_parse_squoted(yytext + 1, yyleng - 1);
	if (!inner) return yytext[0];
	yylval->strval = inner; return LSTREFSYM;
}
[\']([^\\\']|\\.)*[\'] {
	/* plain quoted constructor: 'Constr => LSTSYMBOL */
	const lsstr_t* inner = lslex_parse_squoted(yytext, yyleng);
	if (!inner) return yytext[0];
	yylval->strval = inner; return LSTSYMBOL;
}
[.][a-zA-Z_][a-zA-Z0-9_]* { yylval->strval = lsstr_cstr(yytext); return LSTDOTSYMBOL; }
[!][a-zA-Z_][a-zA-Z0-9_]* {
	/* !ident => LSTENVOP (used to access (~prelude .env).ident) */
	yylval->strval = lsstr_cstr(yytext + 1);
	return LSTENVOP;
}
[~]{2}[a-zA-Z_][a-zA-Z0-9_]* {
	/* ~~symbol => LSTPRELUDE_SYMBOL (plain name, no leading dot) */
	const char* s = yytext + 2;
	yylval->strval = lsstr_cstr(s);
	return LSTPRELUDE_SYMBOL;
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
"||"       { return LSTOROR; }
"^|"       { return LSTCARETBAR; }
"^"        { return LSTCARET; }
. { return yytext[0]; }
<<EOF>> {
		/* When reaching EOF of an included file, pop buffer and restore previous filename. */
				FILE* f = lsscan_pop_file_fp(yyget_extra(yyscanner));
			const char* prev = lsscan_pop_filename(yyget_extra(yyscanner));
		if (prev) {
			yypop_buffer_state(yyscanner);
				if (f) fclose(f);
					lsscan_pop_include_site(yyget_extra(yyscanner));
			/* yylloc filename will be updated on next token via YY_USER_ACTION */
			YY_FLUSH_BUFFER;
			BEGIN(INITIAL);
			YY_BREAK;
		}
		return 0; /* true EOF */
}
%%
#pragma once

#include "array.h"
#include "prog.h"
#include <stdio.h>

typedef struct lsscan {
  const char *filename;
  lsprog_t *prog;
} lsscan_t;

#include "parser.h"

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc, yyscan_t scanner);
void yyerror(YYLTYPE *yylloc, yyscan_t scanner, const char *s);

void lsprintf(FILE *fp, int indent, const char *fmt, ...);

enum {
  LSPREC_LOWEST = 0,
  LSPREC_CONS,
  LSPREC_LAMBDA,
  LSPREC_APPL,
};
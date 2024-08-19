#pragma once

#include "array.h"
#include <stdio.h>

typedef struct lsscan {
  const char *filename;
} lsscan_t;

#include "parser.h"

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc, yyscan_t scanner);
void yyerror(YYLTYPE *yylloc, yyscan_t scanner, const char *s);
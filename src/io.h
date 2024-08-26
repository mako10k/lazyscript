#pragma once

#include "loc.h"
#include <stdio.h>

void lsprintf(FILE *fp, int indent, const char *fmt, ...);

void yyerror(lsloc_t *loc, lsscan_t *scanner, const char *s);
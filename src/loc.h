#pragma once

#include <stdio.h>

typedef struct lsloc lsloc_t;

typedef struct lsloc {
  const char *filename;
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} lsloc_t;

#include "lazyscript.h"

lsloc_t lsloc(const char *filename, int first_line, int first_column,
              int last_line, int last_column);

void lsloc_update(lsloc_t *loc, const char *buf, int len);

void lsloc_print(FILE *fp, lsloc_t loc);

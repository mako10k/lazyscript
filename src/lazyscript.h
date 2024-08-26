#pragma once

#include "array.h"
#include "prog.h"
#include <stdio.h>

typedef struct lsscan {
  lsprog_t *prog;
  const char *filename;
} lsscan_t;

enum {
  LSPREC_LOWEST = 0,
  LSPREC_CONS,
  LSPREC_LAMBDA,
  LSPREC_APPL,
};

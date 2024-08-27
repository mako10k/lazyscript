#pragma once

#include "prog.h"
#include <stdio.h>

typedef struct lsscan {
  lsprog_t *prog;
  const char *filename;
} lsscan_t;
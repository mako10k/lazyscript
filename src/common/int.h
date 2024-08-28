#pragma once

typedef struct lsint lsint_t;

#include "lstypes.h"
#include <stdio.h>

lsint_t *lsint_new(int val);
void lsint_print(FILE *fp, lsprec_t prec, int indent, const lsint_t *val);
int lsint_eq(const lsint_t * val1, const lsint_t *val2);
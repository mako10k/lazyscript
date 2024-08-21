#pragma once

typedef struct lsint lsint_t;

#include <stdio.h>

lsint_t *lsint(int intval);
void lsint_print(FILE *fp, int prec, int indent, const lsint_t *intval);
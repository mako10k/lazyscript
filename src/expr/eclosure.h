#pragma once

typedef struct lseclosure lseclosure_t;

#include "expr/expr.h"
#include "misc/bind.h"

lseclosure_t *lseclosure_new(lsexpr_t *expr, lsbind_t *bind);
void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                      lseclosure_t *eclosure);
lspres_t lseclosure_prepare(lseclosure_t *eclosure, lseenv_t *eenv);
#pragma once

typedef struct lseclosure lseclosure_t;

#include "expr/expr.h"
#include "misc/bind.h"

const lseclosure_t *lseclosure_new(const lsexpr_t *expr, const lsbind_t *bind);
void lseclosure_print(FILE *stream, lsprec_t prec, int indent,
                      const lseclosure_t *eclosure);
lspres_t lseclosure_prepare(const lseclosure_t *eclosure, lseenv_t *eenv);
#pragma once

typedef struct lseclosure lseclosure_t;

#include "bind.h"
#include "expr.h"
#include "tenv.h"
#include "thunk.h"

lseclosure_t *lseclosure(lsexpr_t *expr, lsbind_t *bind);

void lseclosure_print(FILE *stream, int prec, int indent,
                      lseclosure_t *eclosure);
int lseclosure_prepare(lseclosure_t *eclosure, lseenv_t *eenv);
lsthunk_t *lseclosure_thunk(lstenv_t *tenv, const lseclosure_t *eclosure);

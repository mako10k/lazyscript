#pragma once

typedef struct lstref lstref_t;

#include "eref.h"
#include "tenv.h"

lstref_t *lstref_new(lstenv_t *tenv, const lseref_t *eref);
lsthunk_t *lstref_eval(lstref_t *tref);
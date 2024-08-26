#pragma once

typedef struct lstref lstref_t;

#include "eref.h"
#include "tenv.h"

lstref_t *lstref(lstenv_t *tenv, const lseref_t *eref);
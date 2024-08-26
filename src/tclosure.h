#pragma once

typedef struct lstclosure lstclosure_t;

#include "eclosure.h"
#include "tenv.h"

lstclosure_t *lstclosure(lstenv_t *tenv, const lseclosure_t *eclosure);
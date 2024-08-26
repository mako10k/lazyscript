#pragma once

typedef struct lstappl lstappl_t;

#include "eappl.h"
#include "tenv.h"
#include "thunk.h"

lstappl_t *lstappl(lstenv_t *tenv, const lseappl_t *eappl);
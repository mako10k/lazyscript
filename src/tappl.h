#pragma once

typedef struct lstappl lstappl_t;

#include "eappl.h"
#include "tenv.h"
#include "thunk.h"
#include "tlist.h"

lstappl_t *lstappl_new(lstenv_t *tenv, const lseappl_t *eappl);
lsthunk_t *lstappl_eval(lstappl_t *tappl);
lsthunk_t *lstappl_apply(lstappl_t *tappl, const lstlist_t *args);
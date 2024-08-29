#pragma once

typedef struct lstappl lstappl_t;

#include "expr/eappl.h"
#include "thunk/thunk.h"
#include "thunk/tlist.h"
#include "thunk/tenv.h"

lstappl_t *lstappl_new(const lseappl_t *eappl, lstenv_t *tenv);
lsthunk_t *lstappl_eval(lstappl_t *tappl);
lsthunk_t *lstappl_apply(lstappl_t *tappl, const lstlist_t *args);
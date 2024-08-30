#pragma once

typedef struct lstchoice lstchoice_t;

#include "expr/echoice.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"
#include "thunk/tlist.h"

lstchoice_t *lstchoice_new(const lsechoice_t *lechoice, lstenv_t *tenv);
lsthunk_t *lstchoice_get_left(const lstchoice_t *choice);
lsthunk_t *lstchoice_get_right(const lstchoice_t *choice);
lsthunk_t *lstchoice_eval(lstchoice_t *choice);
lsthunk_t *lstchoice_apply(lstchoice_t *choice, const lstlist_t *args);
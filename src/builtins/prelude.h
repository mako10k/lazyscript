#pragma once

#include "thunk/tenv.h"
#include "thunk/thunk.h"

// Host registration for prelude dispatcher builtin
void ls_register_builtin_prelude(lstenv_t* tenv);

// Provided by host (currently lazyscript.c): implementation of prelude.require
lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);

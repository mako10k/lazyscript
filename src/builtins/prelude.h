#pragma once

#include "thunk/tenv.h"
#include "thunk/thunk.h"

// Host registration for prelude dispatcher builtin

// Provided by host (currently lazyscript.c): implementation of prelude.require
lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);

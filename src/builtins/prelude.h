#pragma once

#include "thunk/tenv.h"
#include "thunk/thunk.h"

// Prelude related builtins (host/runtime側で提供)
lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);

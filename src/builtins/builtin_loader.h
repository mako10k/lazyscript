#pragma once

#include "thunk/thunk.h"

// (~prelude builtin) "name" => namespace value (builtin module)
lsthunk_t* lsbuiltin_prelude_builtin(lssize_t argc, lsthunk_t* const* args, void* data);

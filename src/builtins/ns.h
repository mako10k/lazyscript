#pragma once

#include "thunk/thunk.h"

// Namespace builtins
lsthunk_t* lsbuiltin_nsnew(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data);

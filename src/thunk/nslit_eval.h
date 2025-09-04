#pragma once

#include "thunk/thunk.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function pointer type for core NSLIT evaluator
typedef lsthunk_t* (*ls_nslit_eval_fn)(lssize_t argc, lsthunk_t* const* args, void* data);

// Register a core NSLIT evaluator (to be called by frontends like lazyscript.c)
void ls_register_nslit_eval(ls_nslit_eval_fn fn, void* data);

// Call the registered evaluator; returns error thunk if not registered
lsthunk_t* ls_call_nslit_eval(lssize_t argc, lsthunk_t* const* args);

#ifdef __cplusplus
}
#endif

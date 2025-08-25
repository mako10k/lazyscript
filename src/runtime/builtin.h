#pragma once

#include "thunk/thunk.h"
#include "thunk/tenv.h"

// Core builtin function prototypes (centralized)

// printing/string
lsthunk_t* lsbuiltin_dump(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_print(lssize_t argc, lsthunk_t* const* args, void* data);

// sequencing
lsthunk_t* lsbuiltin_seq(lssize_t argc, lsthunk_t* const* args, void* data);

// arithmetic
lsthunk_t* lsbuiltin_add(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_sub(lssize_t argc, lsthunk_t* const* args, void* data);

// namespaces
lsthunk_t* lsbuiltin_nsnew(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_nsnew0(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_nsdefv(lssize_t argc, lsthunk_t* const* args, void* data);

// prelude (plugin-only; keep require API symbols for host)
lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_prelude_ns_self(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_prelude_builtin(lssize_t argc, lsthunk_t* const* args, void* data);


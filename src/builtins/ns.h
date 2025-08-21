#pragma once

#include "thunk/thunk.h"

// Namespace builtins
lsthunk_t* lsbuiltin_nsnew(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data);
// Anonymous namespace helpers
lsthunk_t* lsbuiltin_nsnew0(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_nsdefv(lssize_t argc, lsthunk_t* const* args, void* data);
// Pure namespace literal
lsthunk_t* lsbuiltin_nslit(lssize_t argc, lsthunk_t* const* args, void* data);
// Expose namespace value dispatcher
lsthunk_t* lsbuiltin_ns_value(lssize_t argc, lsthunk_t* const* args, void* data);
// Enumerate namespace members in stable lexicographic order (symbols only)
lsthunk_t* lsbuiltin_ns_members(lssize_t argc, lsthunk_t* const* args, void* data);
// Expose prelude.nsSelf (available only within nslit evaluation)
lsthunk_t* lsbuiltin_prelude_ns_self(lssize_t argc, lsthunk_t* const* args, void* data);

// Internal helper: iterate members of a namespace (value or named symbol)
// For each member, callback receives the symbol name (without internal tag, may start with '.')
// and the bound value thunk.
typedef void (*lsns_iter_cb)(const lsstr_t* symbol, lsthunk_t* value, void* data);
int lsns_foreach_member(lsthunk_t* ns_thunk, lsns_iter_cb cb, void* data);

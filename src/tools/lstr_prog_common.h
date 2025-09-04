#ifndef LS_TOOLS_LSTR_PROG_COMMON_H
#define LS_TOOLS_LSTR_PROG_COMMON_H

#include "lstr/lstr.h"
#include "thunk/thunk.h"

// Load an LSTI image from path and validate the program.
// On success, *out_prog is set and 0 is returned.
// On failure, prints a brief message to stderr and returns a non-zero code:
//  1: load failed, 2: validation failed
int lsprog_load_validate(const char* path, const lstr_prog_t** out_prog);

// Materialize program roots into thunks for simple inspection.
// Allocates a new lstenv; returns roots/ct via out params.
// On failure, prints a brief message to stderr and returns non-zero (3).
int lsprog_materialize_roots(const lstr_prog_t* prog,
                             lsthunk_t***       out_roots,
                             lssize_t*          out_rootc,
                             lstenv_t**         out_env);

#endif // LS_TOOLS_LSTR_PROG_COMMON_H

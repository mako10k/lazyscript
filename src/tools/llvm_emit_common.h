#ifndef LLVM_EMIT_COMMON_H
#define LLVM_EMIT_COMMON_H

#include <stdio.h>
#include <stddef.h>
#include "lstr/lstr.h"
#include "thunk/thunk.h"

// Common helpers for emitting minimal LLVM IR text

// Module header
void lsemit_ir_prelude(FILE* out);

// Emit a trivial main returning an i32 constant
void lsemit_ir_main_ret_i32(FILE* out, int v);

// Declaration for puts (opaque pointers friendly)
void lsemit_ir_decl_puts(FILE* out);

// Global string constants
void lsemit_ir_global_str(FILE* out, const char* cstr, size_t n);
void lsemit_ir_global_named_str(FILE* out, const char* gname, const char* cstr, size_t n);

// Main that calls puts on @.str and returns 0
void lsemit_ir_main_puts(FILE* out, size_t n);

// Simple name mangling utilities
void lsemit_mangle_ref_name(const lsstr_t* name, char* out, size_t outsz);
void lsemit_mangle_sym_ir_label(const char* s, size_t n, char* out, size_t outsz);

// Helper to bind a named global string into a register and return 0
void lsemit_ir_main_bind_sym_and_ret0(FILE* out, const char* reg, const char* gname, size_t n);

// Convenience emitters for common thunk cases
void lsemit_emit_main_ret_from_int(FILE* out, const lsthunk_t* t);
void lsemit_emit_main_puts_from_str(FILE* out, const lsthunk_t* t);
// For a SYMBOL thunk: emit a private global for its text and bind it in main, then ret 0
// Strips a leading '.' from the symbol text when creating the payload string.
void lsemit_emit_symbol_global_and_main(FILE* out, const lsthunk_t* t);

// External call helpers shared by tools
void lsemit_ir_decl_ext_i32(FILE* out, const char* fname, int argc);
void lsemit_ir_main_call_ext_ret_i32(FILE* out, const char* fname, int argc, const int* args);

#endif // LLVM_EMIT_COMMON_H

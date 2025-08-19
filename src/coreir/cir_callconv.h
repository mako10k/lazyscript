#pragma once

// Shared runtime calling convention (C-ABI) for CoreIR/LLVMIR backends.
// Values are represented by the existing thunk value graph.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsthunk lsrt_value_t;

// Basic constructors
lsrt_value_t *lsrt_make_int(long long v);
lsrt_value_t *lsrt_make_str_n(const char *bytes, unsigned long n);
lsrt_value_t *lsrt_make_str(const char *cstr);
lsrt_value_t *lsrt_make_constr(const char *name, int argc, lsrt_value_t *const *args);

// Unit helper
lsrt_value_t *lsrt_unit(void);

// Apply/evaluate
lsrt_value_t *lsrt_apply(lsrt_value_t *func, int argc, lsrt_value_t *const *args);
lsrt_value_t *lsrt_eval(lsrt_value_t *v);

// Debug/IO helpers (minimal)
// Print value similarly to println (adds newline). Returns unit.
lsrt_value_t *lsrt_println(lsrt_value_t *v);

#ifdef __cplusplus
}
#endif

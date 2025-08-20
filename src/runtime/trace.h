#pragma once

#include <stdio.h>
#include "common/loc.h"
#include "common/str.h"

#ifdef __cplusplus
extern "C" {
#endif

// Trace table mapping sequential node indices to source spans.
typedef struct lstrace_span {
  const char* filename;
  int         first_line;
  int         first_column;
  int         last_line;
  int         last_column;
} lstrace_span_t;

typedef struct lstrace_table {
  lstrace_span_t* spans;   // length = count
  int             count;
} lstrace_table_t;

// Global trace table for current program (nullable)
extern lstrace_table_t* g_lstrace_table;

// Load a JSONL sourcemap file: each line as {"i":N,"src":"file","sl":1,"sc":1,"el":1,"ec":1}
// Returns 0 on success, non-zero on error.
int  lstrace_load_jsonl(const char* path);
void lstrace_free(void);

// Lookup span by index; if not found, returns a default span with filename "<unknown>".
lstrace_span_t lstrace_lookup(int index);

// Print a single stack frame like: file:line:col
void lstrace_print_frame(FILE* fp, lstrace_span_t span);

// --- Lightweight runtime context (experimental) ---
// Returns current trace id from evaluation context or -1 if unknown
int  lstrace_current(void);
// Push/pop current trace id (nesting-safe)
void lstrace_push(int id);
void lstrace_pop(void);

// Print up to max_depth frames from the current stack (top-first), each prefixed with " at ".
void lstrace_print_stack(FILE* fp, int max_depth);

// --- Optional sidecar generation (experimental) ---
// Begin dumping a JSONL sourcemap to the given path. Returns 0 on success.
int  lstrace_begin_dump(const char* path);
// Emit one location record in creation order. Safe to call when dump is disabled (no-op).
void lstrace_emit_loc(lsloc_t loc);
// Finish dumping and close the file.
void lstrace_end_dump(void);

// Pending source location for next thunk creation (set by lsthunk_new_expr)
void   lstrace_set_pending_loc(lsloc_t loc);
// Take pending loc if any; otherwise return <unknown>
lsloc_t lstrace_take_pending_or_unknown(void);

#ifdef __cplusplus
}
#endif

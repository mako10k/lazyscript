#ifndef LAZYSCRIPT_PLUGIN_COMMON_H
#define LAZYSCRIPT_PLUGIN_COMMON_H

#include <stdlib.h>
#include "common/io.h"
#include "runtime/trace.h"

static inline int plugin_trace_enabled(void) {
  const char* v = getenv("LAZYSCRIPT_ENABLE_TRACE");
  return (v && v[0] && v[0] != '0');
}

static inline void plugin_trace_prefix(const char* tag) {
  int tid = lstrace_current();
  if (tid >= 0) {
    lstrace_span_t s = lstrace_lookup(tid);
    lsprintf(stderr, 0, "[%s] %s:%d:%d-: ", tag,
             s.filename ? s.filename : "<unknown>", s.first_line, s.first_column);
  } else {
    lsprintf(stderr, 0, "[%s] <no-loc>: ", tag);
  }
}

#endif // LAZYSCRIPT_PLUGIN_COMMON_H

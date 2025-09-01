#pragma once

#include "expr/ealge.h"
#include "expr/expr.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/loc.h"
#include "runtime/trace.h"

// Deprecated: #err is abolished. Use bottom.
static inline lsthunk_t* ls_make_err(const char* msg) { return lsthunk_bottom_here(msg); }
static inline int        lsthunk_is_err(const lsthunk_t* thunk) { return lsthunk_is_bottom(thunk); }

static inline lsthunk_t* ls_eval_arg(lsthunk_t* arg, const char* msg) {
  lsthunk_t* v = lsthunk_eval0(arg);
  if (!v)
    return ls_make_err(msg);
  if (lsthunk_is_err(v))
    return v;
  return v;
}
// Convenience for NoMatch bottom
static inline lsthunk_t* ls_make_bottom_nomatch(const char* info) {
  return lsthunk_new_bottom(info ? info : "NoMatchPatterns", lstrace_take_pending_or_unknown(), 0,
                            NULL);
}

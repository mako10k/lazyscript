#pragma once

#include "expr/ealge.h"
#include "expr/expr.h"
#include "thunk/thunk.h"
#include "common/str.h"

static inline lsthunk_t* ls_make_err(const char* msg) {
  const lsstr_t* s = lsstr_cstr(msg);
  const lsexpr_t* arg = lsexpr_new_str(s);
  const lsexpr_t* args[1] = { arg };
  const lsealge_t* eerr = lsealge_new(lsstr_cstr("#err"), 1, args);
  return lsthunk_new_ealge(eerr, NULL);
}

static inline int lsthunk_is_err(const lsthunk_t* thunk) {
  return thunk && lsthunk_get_type(thunk) == LSTTYPE_ALGE &&
         lsstrcmp(lsthunk_get_constr(thunk), lsstr_cstr("#err")) == 0;
}

static inline lsthunk_t* ls_eval_arg(lsthunk_t* arg, const char* msg) {
  lsthunk_t* v = lsthunk_eval0(arg);
  if (!v) return ls_make_err(msg);
  if (lsthunk_is_err(v)) return v;
  return v;
}

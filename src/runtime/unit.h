#pragma once

#include "expr/ealge.h"
#include "thunk/thunk.h"
#include "common/str.h"

static inline lsthunk_t* ls_make_unit(void) {
  const lsealge_t* eunit = lsealge_new(lsstr_cstr("()"), 0, NULL);
  return lsthunk_new_ealge(eunit, NULL);
}

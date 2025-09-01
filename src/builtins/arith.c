#ifndef LS_TRACE
#define LS_TRACE 0
#endif
#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/error.h"

lsthunk_t* lsbuiltin_add(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
#if LS_TRACE
  lsprintf(stderr, 0, "DBG add: begin\n");
#endif
  lsthunk_t* lhs = ls_eval_arg(args[0], "add: arg1");
  if (lsthunk_is_err(lhs))
    return lhs;
  lsthunk_t* rhs = ls_eval_arg(args[1], "add: arg2");
  if (lsthunk_is_err(rhs))
    return rhs;
  if (!lhs || !rhs) {
#if LS_TRACE
    lsprintf(stderr, 0, "DBG add: arg eval NULL\n");
#endif
    return ls_make_err("add: arg eval");
  }
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT) {
    return ls_make_err("add: invalid type");
  }
  lsthunk_t* ret = lsthunk_new_int(lsint_add(lsthunk_get_int(lhs), lsthunk_get_int(rhs)));
#if LS_TRACE
  lsprintf(stderr, 0, "DBG add: ok\n");
#endif
  return ret;
}

lsthunk_t* lsbuiltin_sub(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  lsthunk_t* lhs = ls_eval_arg(args[0], "sub: arg1");
  if (lsthunk_is_err(lhs))
    return lhs;
  lsthunk_t* rhs = ls_eval_arg(args[1], "sub: arg2");
  if (lsthunk_is_err(rhs))
    return rhs;
  if (!lhs || !rhs)
    return ls_make_err("sub: arg eval");
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT) {
    return ls_make_err("sub: invalid type");
  }
  return lsthunk_new_int(lsint_sub(lsthunk_get_int(lhs), lsthunk_get_int(rhs)));
}

// Integer less-than: returns true/false for ints; otherwise error
lsthunk_t* lsbuiltin_lt(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  lsthunk_t* lhs = ls_eval_arg(args[0], "lt: arg1");
  if (lsthunk_is_err(lhs))
    return lhs;
  lsthunk_t* rhs = ls_eval_arg(args[1], "lt: arg2");
  if (lsthunk_is_err(rhs))
    return rhs;
  if (!lhs || !rhs)
    return ls_make_err("lt: arg eval");
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT)
    return ls_make_err("lt: invalid type");
  int            a   = lsint_get(lsthunk_get_int(lhs));
  int            b   = lsint_get(lsthunk_get_int(rhs));
  const lsstr_t* res = (a < b) ? lsstr_cstr("true") : lsstr_cstr("false");
  return lsthunk_new_ealge(lsealge_new(res, 0, NULL), NULL);
}

#ifndef LS_TRACE
#define LS_TRACE 0
#endif
#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"

lsthunk_t* lsbuiltin_add(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG add: begin\n");
  #endif
  lsthunk_t* lhs = lsthunk_eval0(args[0]);
  lsthunk_t* rhs = lsthunk_eval0(args[1]);
  if (!lhs || !rhs) { 
    #if LS_TRACE
    lsprintf(stderr, 0, "DBG add: arg eval NULL\n");
    #endif
    return NULL; }
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: add: invalid type\n");
    return NULL;
  }
  lsthunk_t* ret = lsthunk_new_int(lsint_add(lsthunk_get_int(lhs), lsthunk_get_int(rhs)));
  #if LS_TRACE
  lsprintf(stderr, 0, "DBG add: ok\n");
  #endif
  return ret;
}

lsthunk_t* lsbuiltin_sub(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; (void)argc;
  lsthunk_t* lhs = lsthunk_eval0(args[0]);
  lsthunk_t* rhs = lsthunk_eval0(args[1]);
  if (!lhs || !rhs) return NULL;
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: sub: invalid type\n");
    return NULL;
  }
  return lsthunk_new_int(lsint_sub(lsthunk_get_int(lhs), lsthunk_get_int(rhs)));
}

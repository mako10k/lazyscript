#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/error.h"

lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data);

lsthunk_t* lsbuiltin_print(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0,
             "E: print: effect used in pure context (enable seq/chain or run with effects)\n");
    return NULL;
  }
  lsthunk_t* thunk_str = ls_eval_arg(args[0], "print: arg");
  if (lsthunk_is_err(thunk_str))
    return thunk_str;
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR)
    thunk_str = lsbuiltin_to_string(1, args, data);
  if (lsthunk_is_err(thunk_str))
    return thunk_str;
  const lsstr_t* str = lsthunk_get_str(thunk_str);
  lsstr_print_bare(stdout, LSPREC_LOWEST, 0, str);
  return args[0];
}

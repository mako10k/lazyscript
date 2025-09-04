#include "thunk/nslit_eval.h"
#include "runtime/error.h"

static ls_nslit_eval_fn g_nslit_eval = NULL;
static void*            g_nslit_data = NULL;

void ls_register_nslit_eval(ls_nslit_eval_fn fn, void* data) {
  g_nslit_eval = fn;
  g_nslit_data = data;
}

lsthunk_t* ls_call_nslit_eval(lssize_t argc, lsthunk_t* const* args) {
  if (!g_nslit_eval)
    return ls_make_err("nslit: core evaluator not registered");
  return g_nslit_eval(argc, args, g_nslit_data);
}

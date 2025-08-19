#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"

lsthunk_t* lsbuiltin_dump(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  lsthunk_print(stdout, LSPREC_LOWEST, 0, args[0]);
  lsprintf(stdout, 0, "\n");
  return args[0];
}

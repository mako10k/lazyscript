#ifndef LS_TRACE
#define LS_TRACE 0
#endif
#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/error.h"

lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  size_t     len = 0;
  char*      buf = NULL;
  FILE*      fp  = lsopen_memstream_gc(&buf, &len);
  lsthunk_t* v   = ls_eval_arg(args[0], "to_string: arg");
  if (lsthunk_is_err(v)) {
    fclose(fp);
    return v;
  }
  if (!v) {
    fclose(fp);
    return ls_make_err("to_string: arg eval");
  }
  // Deep print to evaluate nested structures so results like Some 3 are rendered
  lsthunk_deep_print(fp, LSPREC_LOWEST, 0, v);
  fclose(fp);
  const lsstr_t* str = lsstr_new(buf, len);
  return lsthunk_new_str(str);
}

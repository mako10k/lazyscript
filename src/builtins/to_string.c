#ifndef LS_TRACE
#define LS_TRACE 0
#endif
#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"

lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  size_t len = 0; char* buf = NULL; FILE* fp = lsopen_memstream_gc(&buf, &len);
  lsthunk_t* v = lsthunk_eval0(args[0]);
  if (!v) { fclose(fp); return NULL; }
  lsthunk_dprint(fp, LSPREC_LOWEST, 0, v);
  fclose(fp);
  const lsstr_t* str = lsstr_new(buf, len);
  return lsthunk_new_str(str);
}

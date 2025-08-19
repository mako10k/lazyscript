// TEMP: enable trace for debugging t30
#undef LS_TRACE
#define LS_TRACE 1
#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"

lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  (void)argc;
  size_t len = 0; char* buf = NULL; FILE* fp = lsopen_memstream_gc(&buf, &len);
  lsthunk_t* v = lsthunk_eval0(args[0]);
  if (!v) { 
    #if LS_TRACE
    lsprintf(stderr, 0, "DBG to_str: arg eval -> NULL\n");
    #endif
    fclose(fp); return NULL; }
  #if LS_TRACE
  {
    const char* vt = "?";
    switch (lsthunk_get_type(v)) {
    case LSTTYPE_INT: vt = "int"; break; case LSTTYPE_STR: vt = "str"; break;
    case LSTTYPE_ALGE: vt = "alge"; break; case LSTTYPE_APPL: vt = "appl"; break;
    case LSTTYPE_LAMBDA: vt = "lambda"; break; case LSTTYPE_REF: vt = "ref"; break;
    case LSTTYPE_CHOICE: vt = "choice"; break; case LSTTYPE_BUILTIN: vt = "builtin"; break; }
    lsprintf(stderr, 0, "DBG to_str: arg type=%s\n", vt);
  }
  #endif
  lsthunk_dprint(fp, LSPREC_LOWEST, 0, v);
  fclose(fp);
  const lsstr_t* str = lsstr_new(buf, len);
  return lsthunk_new_str(str);
}

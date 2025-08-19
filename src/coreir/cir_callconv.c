#include "coreir/cir_callconv.h"
#include "thunk/thunk.h"
#include "expr/ealge.h"
#include "common/str.h"
#include "common/io.h"

lsrt_value_t *lsrt_make_int(long long v) {
  return lsthunk_new_int(lsint_new((int)v));
}

lsrt_value_t *lsrt_make_str_n(const char *bytes, unsigned long n) {
  const lsstr_t *s = lsstr_new(bytes, n);
  return lsthunk_new_str(s);
}

lsrt_value_t *lsrt_make_str(const char *cstr) {
  const lsstr_t *s = lsstr_cstr(cstr);
  return lsthunk_new_str(s);
}

lsrt_value_t *lsrt_make_constr(const char *name, int argc, lsrt_value_t *const *args) {
  const lsealge_t *alge = lsealge_new(lsstr_cstr(name), 0, NULL);
  // Build an ALGE thunk and then append args by applying (reuse existing eval path)
  lsrt_value_t *con = lsthunk_new_ealge(alge, NULL);
  if (argc == 0) return con;
  return lsthunk_eval(con, argc, (lsthunk_t *const *)args);
}

lsrt_value_t *lsrt_unit(void) {
  const lsealge_t *eunit = lsealge_new(lsstr_cstr("()"), 0, NULL);
  return lsthunk_new_ealge(eunit, NULL);
}

lsrt_value_t *lsrt_apply(lsrt_value_t *func, int argc, lsrt_value_t *const *args) {
  return lsthunk_eval(func, argc, (lsthunk_t *const *)args);
}

lsrt_value_t *lsrt_eval(lsrt_value_t *v) { return lsthunk_eval0(v); }

lsrt_value_t *lsrt_println(lsrt_value_t *v) {
  // Call existing println via prelude: (~prelude println) v
  // For now, reuse C helper: convert to string and print newline.
  v = lsthunk_eval0(v);
  if (v == NULL) return NULL;
  if (lsthunk_get_type(v) != LSTTYPE_STR) {
    // to_string builtin exists in main binary; fallback: shallow print
    size_t len = 0; char *buf = NULL; FILE *fp = lsopen_memstream_gc(&buf, &len);
    lsthunk_dprint(fp, LSPREC_LOWEST, 0, v);
    fclose(fp);
    const lsstr_t *s = lsstr_new(buf, len);
    v = lsthunk_new_str(s);
  }
  lsstr_print_bare(stdout, LSPREC_LOWEST, 0, lsthunk_get_str(v));
  lsprintf(stdout, 0, "\n");
  return lsrt_unit();
}

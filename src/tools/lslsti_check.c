#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lazyscript.h"
#include "thunk/thunk.h"
#include "thunk/lsti.h"
#include "expr/expr.h"
#include "expr/ealge.h"
#include "common/malloc.h"
#include "runtime/trace.h"
// Note: We don't need real runtime builtins here; provide a local dummy.

// Local dummy builtin function (never actually evaluated in this smoke)
static lsthunk_t* lsdummy_builtin(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)args;
  (void)data;
  return lsthunk_bottom_here("dummy-builtin should not run in lslsti_check");
}

int main(int argc, char** argv) {
  const char* path = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  // Build a tiny graph: INT 42, STR "hello", SYMBOL ".Ok", ALGE (.Ok 42), BOTTOM("boom",
  // related=[INT 42])
  lstenv_t*      env     = lstenv_new(NULL);
  const lsint_t* i42     = lsint_new(42);
  lsthunk_t*     ti      = lsthunk_new_int(i42);
  const lsstr_t* s_hello = lsstr_cstr("hello");
  lsthunk_t*     ts      = lsthunk_new_str(s_hello);
  const lsstr_t* y_ok    = lsstr_cstr(".Ok");
  lsthunk_t*     ty      = lsthunk_new_symbol(y_ok);
  // ALGE: (.Ok 42) using public APIs
  const lsexpr_t*  arg_e    = lsexpr_new_int(i42);
  const lsexpr_t*  args1[1] = { arg_e };
  const lsealge_t* e_ok     = lsealge_new(lsstr_cstr(".Ok"), 1, args1);
  lsthunk_t*       talge    = lsthunk_new_ealge(e_ok, env);
  // BOTTOM: message and one related using public API
  lsthunk_t* related[1] = { ts };
  lsthunk_t* tbot       = lsthunk_new_bottom("boom", lstrace_take_pending_or_unknown(), 1, related);
  // BUILTIN: prelude.print (arity 1) — writer will encode as REF("prelude.print")
  // Use a local dummy func to avoid linking real builtins.
  lsthunk_t* tbi =
      lsthunk_new_builtin_attr(lsstr_cstr("prelude.print"), 1, lsdummy_builtin, NULL, 0);
  // LAMBDA: \x -> x  （param = REF x, body = REF x）
  const lsref_t*  xr    = lsref_new(lsstr_cstr("x"), lstrace_take_pending_or_unknown());
  lstpat_t*       xpat  = lstpat_new_ref(xr);
  lsthunk_t*      lam   = lsthunk_alloc_lambda(xpat);
  lsthunk_t*      xref  = lsthunk_new_ref(xr, env);
  lsthunk_set_lambda_body(lam, xref);
  lsthunk_t* roots[7]   = { ti, ts, ty, talge, tbot, tbi, lam };

  FILE*      fp = fopen(path, "wb");
  if (!fp) {
    perror("fopen");
    return 1;
  }
  lsti_write_opts_t opt = { .align_log2 = LSTI_ALIGN_8, .flags = 0 };
  int               rc  = lsti_write(fp, roots, 7, &opt);
  fclose(fp);
  if (rc != 0) {
    fprintf(stderr, "lsti_write rc=%d\n", rc);
    return 2;
  }

  lsti_image_t img = { 0 };
  rc               = lsti_map(path, &img);
  if (rc != 0) {
    fprintf(stderr, "lsti_map rc=%d\n", rc);
    return 3;
  }
  rc = lsti_validate(&img);
  if (rc != 0) {
    fprintf(stderr, "lsti_validate rc=%d\n", rc);
    lsti_unmap(&img);
    return 4;
  }
  // Try materialize (now supports INT/STR/SYMBOL/ALGE/BOTTOM/APPL/CHOICE/REF/LAMBDA subset)
  lsthunk_t** out_roots = NULL;
  lssize_t    outc      = 0;
  rc                    = lsti_materialize(&img, &out_roots, &outc, env);
  if (rc == 0) {
    printf("materialize: roots=%ld\n", (long)outc);
  } else {
    printf("materialize: rc=%d (expected for subset)\n", rc);
  }
  lsti_unmap(&img);
  (void)env; // quiet unused for now
  printf("ok: %s\n", path);
  return 0;
}
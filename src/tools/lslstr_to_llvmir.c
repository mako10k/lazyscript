#include <stdio.h>
#include <stdlib.h>
#include "lstr/lstr.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include <ctype.h>
#include <string.h>
#include "tools/llvm_emit_common.h"
#include "tools/lstr_prog_common.h"

// Minimal LSTR -> LLVM IR (text) emitter
// - Input: LSTI image path (default: ./_tmp_test.lsti)
// - Output: LLVM IR textual module to stdout
// Behavior:
//   If root[0] is an INT constant, emit main returning that value.
//   If root[0] is ALGE(.Ok <INT>), return that INT.
//   If root[0] is a STR constant, emit a call to puts and return 0.

// external call helpers are shared via tools/llvm_emit_common.h

int main(int argc, char** argv) {
  const char* path       = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  long        root_index = 0;
  if (argc > 2) {
    char* end = NULL;
    long  v   = strtol(argv[2], &end, 10);
    if (end && *end == '\0' && v >= 0)
      root_index = v;
  }
  const lstr_prog_t* p = NULL;
  if (lsprog_load_validate(path, &p))
    return 1;
  // Materialize minimal roots to inspect constants
  lstenv_t*   env   = NULL;
  lsthunk_t** roots = NULL;
  lssize_t    rootc = 0;
  if (lsprog_materialize_roots(p, &roots, &rootc, &env))
    return 3;
  lsemit_ir_prelude(stdout);
  if (rootc > 0 && roots && root_index < rootc && roots[root_index]) {
    lsthunk_t* r0 = roots[root_index];
    switch (lsthunk_get_type(r0)) {
    case LSTTYPE_INT: {
      lsemit_emit_main_ret_from_int(stdout, r0);
      break;
    }
    case LSTTYPE_ALGE: {
      const lsstr_t* c = lsthunk_get_constr(r0);
      if (c && lsstrcmp(c, lsstr_cstr(".Ok")) == 0 && lsthunk_get_argc(r0) == 1) {
        lsthunk_t* const* args = lsthunk_get_args(r0);
        if (args && args[0] && lsthunk_get_type(args[0]) == LSTTYPE_INT) {
          const lsint_t* iv   = lsthunk_get_int(args[0]);
          int            retv = iv ? lsint_get(iv) : 0;
          lsemit_ir_main_ret_i32(stdout, retv);
          break;
        }
      }
      // Fallback
      lsemit_ir_main_ret_i32(stdout, 0);
      break;
    }
    case LSTTYPE_STR: {
      lsemit_emit_main_puts_from_str(stdout, r0);
      break;
    }
    case LSTTYPE_REF: {
      const lsstr_t* nm = lsthunk_get_ref_name(r0);
      if (nm) {
        char m[256];
        lsemit_mangle_ref_name(nm, m, sizeof m);
  lsemit_ir_decl_ext_i32(stdout, m, 0);
  lsemit_ir_main_call_ext_ret_i32(stdout, m, 0, NULL);
        break;
      }
      lsemit_ir_main_ret_i32(stdout, 0);
      break;
    }
    case LSTTYPE_APPL: {
      lsthunk_t* fn = lsthunk_get_appl_func(r0);
      if (fn && lsthunk_get_type(fn) == LSTTYPE_REF) {
        const lsstr_t*    nm = lsthunk_get_ref_name(fn);
        lssize_t          ac = lsthunk_get_argc(r0);
        lsthunk_t* const* av = lsthunk_get_args(r0);
        // Only support all-int arguments for now
        int ok = 1;
        for (lssize_t i = 0; i < ac; i++) {
          if (!(av && av[i] && lsthunk_get_type(av[i]) == LSTTYPE_INT)) {
            ok = 0;
            break;
          }
        }
        if (ok && nm) {
          int bufc = (ac > 0 && ac < 16) ? (int)ac : 0; // cap for stack array safety
          int argv[16];
          for (int i = 0; i < bufc; i++) {
            const lsint_t* iv = lsthunk_get_int(av[i]);
            argv[i]           = iv ? lsint_get(iv) : 0;
          }
          char m[256];
          lsemit_mangle_ref_name(nm, m, sizeof m);
          lsemit_ir_decl_ext_i32(stdout, m, bufc);
          lsemit_ir_main_call_ext_ret_i32(stdout, m, bufc, argv);
          break;
        }
      }
      lsemit_ir_main_ret_i32(stdout, 0);
      break;
    }
    default:
      lsemit_ir_main_ret_i32(stdout, 0);
    }
  } else {
    lsemit_ir_main_ret_i32(stdout, 0);
  }
  return 0;
}

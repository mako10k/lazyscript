#include <stdio.h>
#include <stdlib.h>
#include "lstr/lstr.h"
#include "thunk/thunk.h"

// Minimal LSTR -> LLVM IR (text) emitter
// - Input: LSTI image path (default: ./_tmp_test.lsti)
// - Output: LLVM IR textual module to stdout
// Behavior:
//   If root[0] is an INT constant, emit main returning that value; otherwise return 0.

static void emit_ir_prelude(FILE* out){
  fprintf(out, "; ModuleID = 'lazyscript'\n");
  fprintf(out, "source_filename = \"lazyscript\"\n\n");
}

static void emit_ir_main_ret_i32(FILE* out, int v){
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  ret i32 %d\n", v);
  fprintf(out, "}\n");
}

int main(int argc, char** argv){
  const char* path = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  const lstr_prog_t* p = lstr_from_lsti_path(path);
  if(!p){
    fprintf(stderr, "failed: lstr_from_lsti_path(%s)\n", path);
    return 1;
  }
  if(lstr_validate(p)!=0){
    fprintf(stderr, "invalid LSTR\n");
    return 2;
  }
  // Materialize minimal roots to inspect constants
  lstenv_t* env = lstenv_new(NULL);
  lsthunk_t** roots = NULL; lssize_t rootc = 0;
  if(lstr_materialize_to_thunks(p, &roots, &rootc, env)!=0){
    fprintf(stderr, "materialize failed\n");
    return 3;
  }
  int retv = 0;
  if(rootc > 0 && roots && roots[0]){
    lsthunk_t* r0 = roots[0];
    if(lsthunk_get_type(r0) == LSTTYPE_INT){
      const lsint_t* iv = lsthunk_get_int(r0);
      retv = iv ? lsint_get(iv) : 0;
    }
  }
  emit_ir_prelude(stdout);
  emit_ir_main_ret_i32(stdout, retv);
  return 0;
}

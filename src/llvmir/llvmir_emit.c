#include "llvmir/llvmir.h"
#include "coreir/coreir.h"
#include "coreir/cir_internal.h"
#include <stdio.h>

// Very small placeholder: emit an LLVM IR module with a main that returns 0
// Later this will lower Core IR expressions to functions and calls.
int lsllvm_emit_text(FILE* out, const lscir_prog_t* cir) {
  (void)cir;
  fprintf(out, "; ModuleID = 'lazyscript_mvp'\n"
               "source_filename = \"lazyscript_mvp\"\n\n"
               "define i32 @main() {\n"
               "entry:\n"
               "  ret i32 0\n"
               "}\n");
  return 0;
}

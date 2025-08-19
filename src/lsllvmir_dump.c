#include "lazyscript.h"
#include "coreir/coreir.h"
#include "llvmir/llvmir.h"
#include "parser/parser.h"
#include "parser/lexer.h"
#include <stdio.h>

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  yyscan_t yyscanner; yylex_init(&yyscanner);
  lsscan_t *scan = lsscan_new("<stdin>");
  yyset_in(stdin, yyscanner);
  yyset_extra(scan, yyscanner);
  int ret = yyparse(yyscanner);
  const lsprog_t *prog = ret == 0 ? lsscan_get_prog(scan) : NULL;
  yylex_destroy(yyscanner);
  if (!prog) return 1;
  const lscir_prog_t *cir = lscir_lower_prog(prog);
  return lsllvm_emit_text(stdout, cir);
}

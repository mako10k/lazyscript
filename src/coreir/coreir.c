#include "coreir/coreir.h"
#include "common/malloc.h"
#include "misc/prog.h"

struct lscir_prog {
  const lsprog_t *ast;
};

const lscir_prog_t *lscir_lower_prog(const lsprog_t *prog) {
  lscir_prog_t *cir = lsmalloc(sizeof(lscir_prog_t));
  cir->ast = prog;
  return cir;
}

void lscir_print(FILE *fp, int indent, const lscir_prog_t *cir) {
  // Temporary: just print the surface AST under a [CoreIR] heading
  fprintf(fp, "; CoreIR dump (temporary)\n");
  const lsprog_t *ast = cir->ast;
  if (ast) {
    lsprog_print(fp, 0, indent, ast);
  } else {
    fprintf(fp, "<null>\n");
  }
}

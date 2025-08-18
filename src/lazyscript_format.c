#include "lazyscript.h"
#include "common/io.h"
#include "parser/parser.h"
#include "parser/lexer.h"
#include <stdio.h>
#include <stdlib.h>

// Local parse helper (duplicated from lazyscript.c)
static const lsprog_t *lsparse_stream_local(const char *filename, FILE *in_str) {
  yyscan_t yyscanner;
  yylex_init(&yyscanner);
  lsscan_t *lsscan = lsscan_new(filename);
  yyset_in(in_str, yyscanner);
  yyset_extra(lsscan, yyscanner);
  int ret = yyparse(yyscanner);
  const lsprog_t *prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
  yylex_destroy(yyscanner);
  return prog;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input.ls>\n", argv[0]);
    return 1;
  }
  const char *filename = argv[1];
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror(filename);
    return 1;
  }
  const lsprog_t *prog = lsparse_stream_local(filename, fp);
  fclose(fp);
  if (!prog) {
    fprintf(stderr, "Parse error in %s\n", filename);
    return 2;
  }
  lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
  return 0;
}

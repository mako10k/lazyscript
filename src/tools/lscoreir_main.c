#include "lazyscript.h"
#include "parser/parser.h"
#include "parser/lexer.h"
#include "coreir/coreir.h"
#include "runtime/effects.h"
#include <getopt.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>

static const lsprog_t* parse_file(const char* path) {
  FILE* fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    return NULL;
  }
  yyscan_t yyscanner;
  yylex_init(&yyscanner);
  lsscan_t* lsscan = lsscan_new(path);
  yyset_in(fp, yyscanner);
  yyset_extra(lsscan, yyscanner);
  int             ret  = yyparse(yyscanner);
  const lsprog_t* prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
  yylex_destroy(yyscanner);
  fclose(fp);
  return prog;
}

int main(int argc, char** argv) {
  int           strict     = 0;
  int           from_ls    = 0;
  int           debug      = 0;
  struct option longopts[] = { { "strict-effects", no_argument, NULL, 's' },
                               { "from-ls", required_argument, NULL, 1000 },
                               { "debug", no_argument, NULL, 'd' },
                               { "help", no_argument, NULL, 'h' },
                               { 0, 0, 0, 0 } };
  int           opt;
  const char*   from_ls_path = NULL;
  while ((opt = getopt_long(argc, argv, "sdh", longopts, NULL)) != -1) {
    switch (opt) {
    case 's':
      strict = 1;
      break;
    case 1000:
      from_ls      = 1;
      from_ls_path = optarg;
      break;
    case 'd':
      debug = 1;
      (void)debug;
      break;
    case 'h':
      printf("Usage: %s [--from-ls FILE] [--strict-effects]\n", argv[0]);
      return 0;
    default:
      break;
    }
  }
  const char* _ls_use_libc = getenv("LAZYSCRIPT_USE_LIBC_ALLOC");
  if (!(_ls_use_libc && _ls_use_libc[0] && _ls_use_libc[0] != '0'))
    GC_init();
  if (strict)
    ls_effects_set_strict(1);

  if (from_ls) {
    const lsprog_t* prog = parse_file(from_ls_path);
    if (!prog)
      return 1;
    const lscir_prog_t* cir = lscir_lower_prog(prog);
    if (strict) {
      int errs = lscir_validate_effects(stderr, cir);
      if (errs > 0) {
        fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
        return 1;
      }
    }
    lscir_eval(stdout, cir);
    return 0;
  }

  // Fallback: read Core IR text from stdin and execute
  const lscir_prog_t* cir = lscir_read_text(stdin, stderr);
  if (!cir)
    return 2;
  if (strict) {
    int errs = lscir_validate_effects(stderr, cir);
    if (errs > 0) {
      fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
      return 1;
    }
  }
  return lscir_eval(stdout, cir);
}

#include "lazyscript.h"
#include "parser/parser.h"
#include "parser/lexer.h"
#include "coreir/coreir.h"
#include "runtime/effects.h"
#include <getopt.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const lsprog_t* parse_stdin(const char* name) {
  yyscan_t yyscanner; yylex_init(&yyscanner);
  lsscan_t* lsscan = lsscan_new(name);
  yyset_in(stdin, yyscanner);
  yyset_extra(lsscan, yyscanner);
  int ret = yyparse(yyscanner);
  const lsprog_t* prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
  yylex_destroy(yyscanner);
  return prog;
}

static const lsprog_t* parse_file(const char* path) {
  FILE* fp = fopen(path, "r"); if (!fp) { perror(path); return NULL; }
  yyscan_t yyscanner; yylex_init(&yyscanner);
  lsscan_t* lsscan = lsscan_new(path);
  yyset_in(fp, yyscanner);
  yyset_extra(lsscan, yyscanner);
  int ret = yyparse(yyscanner);
  const lsprog_t* prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
  yylex_destroy(yyscanner);
  fclose(fp);
  return prog;
}

static const lsprog_t* parse_from_string(const char* name, const char* str) {
  FILE* fp = fmemopen((void*)str, strlen(str), "r");
  if (!fp) return NULL;
  yyscan_t yyscanner; yylex_init(&yyscanner);
  lsscan_t* lsscan = lsscan_new(name);
  yyset_in(fp, yyscanner);
  yyset_extra(lsscan, yyscanner);
  int ret = yyparse(yyscanner);
  const lsprog_t* prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
  yylex_destroy(yyscanner);
  fclose(fp);
  return prog;
}

int main(int argc, char** argv) {
  const char* eval_str = NULL;
  int do_typecheck = 0; int debug = 0; int strict = 0;
  struct option longopts[] = {
    {"eval", required_argument, NULL, 'e'},
    {"typecheck", no_argument, NULL, 't'},
    {"strict-effects", no_argument, NULL, 's'},
    {"debug", no_argument, NULL, 'd'},
    {"help", no_argument, NULL, 'h'},
    {0,0,0,0}
  };
  int opt; while ((opt = getopt_long(argc, argv, "e:tsdh", longopts, NULL)) != -1) {
    switch (opt) {
      case 'e': eval_str = optarg; break;
      case 't': do_typecheck = 1; break;
      case 's': strict = 1; break;
      case 'd': debug = 1; (void)debug; break;
      case 'h':
        printf("Usage: %s [--typecheck|-t] [--strict-effects|-s] [FILE|-e STR]\n", argv[0]);
        return 0;
      default: break;
    }
  }
  const char* _ls_use_libc = getenv("LAZYSCRIPT_USE_LIBC_ALLOC");
  if (!(_ls_use_libc && _ls_use_libc[0] && _ls_use_libc[0] != '0')) GC_init();
  if (strict) ls_effects_set_strict(1);

  const lsprog_t* prog = NULL;
  if (eval_str) {
    char name[64]; snprintf(name, sizeof(name), "<lazyscriptc-eval>");
  prog = parse_from_string(name, eval_str);
  } else if (optind < argc) {
    prog = parse_file(argv[optind]);
  } else {
    prog = parse_stdin("<stdin>");
  }
  if (!prog) return 1;

  const lscir_prog_t* cir = lscir_lower_prog(prog);
  if (strict) {
    int errs = lscir_validate_effects(stderr, cir);
    if (errs > 0) { fprintf(stderr, "E: strict-effects: %d error(s)\n", errs); return 1; }
  }
  if (do_typecheck) {
    return lscir_typecheck(stdout, cir);
  }
  lscir_print(stdout, 0, cir);
  return 0;
}

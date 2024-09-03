#include "lazyscript.h"
#include "misc/prog.h"
#include "parser/parser.h"
#include "thunk/thunk.h"
#include <string.h>

#include "parser/lexer.h"
#include <assert.h>
#include <gc.h>
#include <getopt.h>
#include <stdarg.h>

static const lsprog_t *lsparse_stream(const char *filename, FILE *in_str) {
  assert(in_str != NULL);
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

static const lsprog_t *lsparse_file(const char *filename) {
  FILE *stream = fopen(filename, "r");
  if (!stream) {
    perror(filename);
    exit(1);
  }
  const lsprog_t *prog = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

static const lsprog_t *lsparse_string(const char *filename, const char *str) {
  FILE *stream = fmemopen((void *)str, strlen(str), "r");
  const lsprog_t *prog = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

int main(int argc, char **argv) {
  struct option longopts[] = {
      {"eval", required_argument, NULL, 'e'},
      {"debug", no_argument, NULL, 'd'},
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0},
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "e:dhv", longopts, NULL)) != -1) {
    switch (opt) {
    case 'e': {
      static int eval_count = 0;
      char name[32];
      snprintf(name, sizeof(name), "<eval:#%d>", ++eval_count);
      const lsprog_t *prog = lsparse_string(name, optarg);
      if (prog != NULL)
        lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
      break;
    }
    case 'd':
#if DEBUG
    {
      extern int yydebug;
      yydebug = 1;
    }
#endif
    break;
    case 'h':
      printf("Usage: %s [OPTION]... [FILE]...\n", argv[0]);
      printf("Options:\n");
      printf("  -d, --debug     print debug information\n");
      printf("  -h, --help      display this help and exit\n");
      printf("  -v, --version   output version information and exit\n");
      exit(0);
    case 'v':
      printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      exit(0);
    default:
      exit(1);
    }
  }
  for (int i = optind; i < argc; i++) {
    const char *filename = argv[i];
    if (strcmp(filename, "-") == 0)
      filename = "/dev/stdin";
    const lsprog_t *prog = lsparse_file(argv[i]);
    if (prog != NULL) {
#ifdef DEBUG
      lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
#endif
      lstenv_t *tenv = lstenv_new(NULL);
      lstbuiltin_func_t lsbuiltin_print;
      lstenv_put_builtin(tenv, lsstr_cstr("print"), 1, lsbuiltin_print, NULL);
      lsprog_eval(prog, tenv);
    }
  }
}

static lsthunk_t *lsbuiltin_print(lssize_t argc, lsthunk_t *const *args,
                                  void *data) {
  assert(args != NULL);
  for (lssize_t i = 0; i < argc; i++) {
    if (i > 0)
      printf(" ");
    lsthunk_print(stdout, LSPREC_LOWEST, 0, args[i]);
  }
  return args[0];
}
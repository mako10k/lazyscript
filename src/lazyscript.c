#include "lazyscript.h"
#include "common/str.h"
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

static lsthunk_t *lsbuiltin_dump(lssize_t argc, lsthunk_t *const *args,
                                 void *data) {
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_print(stdout, LSPREC_LOWEST, 0, args[0]);
  lsprintf(stdout, 0, "\n");
  return args[0];
}

static lsthunk_t *lsbuiltin_to_string(lssize_t argc, lsthunk_t *const *args,
                                      void *data) {
  assert(argc == 1);
  assert(args != NULL);
  size_t len = 0;
  char *buf = NULL;
  FILE *fp = open_memstream(&buf, &len);
  lsthunk_print(fp, LSPREC_LOWEST, 0, args[0]);
  fclose(fp);
  const lsstr_t *str = lsstr_new(buf, len);
  free(buf);
  return lsthunk_new_str(str);
}

static lsthunk_t *lsbuiltin_print(lssize_t argc, lsthunk_t *const *args,
                                  void *data) {
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_t *thunk_str = args[0];
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR)
    thunk_str = lsbuiltin_to_string(1, args, data);
  const lsstr_t *str = lsthunk_get_str(thunk_str);
  lsstr_print_bare(stdout, LSPREC_LOWEST, 0, str);
  return args[0];
}

typedef enum lsseq_type {
  LSSEQ_SIMPLE,
  LSSEQ_CHAIN,
} lsseq_type_t;

static lsthunk_t *lsbuiltin_seq(lssize_t argc, lsthunk_t *const *args,
                                void *data) {
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t *fst = args[0];
  lsthunk_t *snd = args[1];
  lsthunk_t *fst_evaled = lsthunk_eval0(fst);
  if (fst_evaled == NULL)
    return NULL;
  switch ((lsseq_type_t)(intptr_t)data) {
  case LSSEQ_SIMPLE: // Simple seq
    return snd;
  case LSSEQ_CHAIN: { // Chain seq
    lsthunk_t *fst_new = lsthunk_new_builtin(
        lsstr_cstr("seqc"), 2, lsbuiltin_seq, (void *)LSSEQ_CHAIN);
    lsthunk_t *ret = lsthunk_eval(fst_new, 1, &snd);
    return ret;
  }
  }
}

static void lsbuiltin_prelude(lstenv_t *tenv) {
  lstenv_put_builtin(tenv, lsstr_cstr("dump"), 1, lsbuiltin_dump, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("to_str"), 1, lsbuiltin_to_string, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("print"), 1, lsbuiltin_print, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("seq"), 2, lsbuiltin_seq,
                     (void *)LSSEQ_SIMPLE);
  lstenv_put_builtin(tenv, lsstr_cstr("seqc"), 2, lsbuiltin_seq,
                     (void *)LSSEQ_CHAIN);
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
      if (prog != NULL) {
#ifdef DEBUG
        lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
#endif
        lstenv_t *tenv = lstenv_new(NULL);
        lsbuiltin_prelude(tenv);
        lsthunk_t *ret = lsprog_eval(prog, tenv);
        if (ret != NULL) {
          lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
          lsprintf(stdout, 0, "\n");
        }
      }
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
      lsbuiltin_prelude(tenv);
      lsthunk_t *ret = lsprog_eval(prog, tenv);
      if (ret != NULL) {
        lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
        lsprintf(stdout, 0, "\n");
      }
    }
  }
}
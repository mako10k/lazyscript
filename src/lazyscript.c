#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_VERSION "0.0.1"
#define PACKAGE_NAME "lazyscript"
#endif
#include "lazyscript.h"
#include "lexer.h"
#include "parser.h"
#include <assert.h>
#include <gc.h>
#include <getopt.h>
#include <stdarg.h>

static lsprog_t *lsparse_stream(const char *filename, FILE *in_str) {
  assert(in_str != NULL);
  yyscan_t yyscanner;
  yylex_init(&yyscanner);
  lsscan_t lsscan = {filename, NULL};
  yyset_in(in_str, yyscanner);
  yyset_extra(&lsscan, yyscanner);
  int ret = yyparse(yyscanner);
  lsprog_t *prog = ret == 0 ? lsscan.prog : NULL;
  yylex_destroy(yyscanner);
  return prog;
}

static lsprog_t *lsparse_file(const char *filename) {
  FILE *stream = fopen(filename, "r");
  if (!stream) {
    perror(filename);
    exit(1);
  }
  lsprog_t *prog = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

static lsprog_t *lsparse_string(const char *filename, const char *str) {
  FILE *stream = fmemopen((void *)str, strlen(str), "r");
  lsprog_t *prog = lsparse_stream(filename, stream);
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
      lsprog_t *prog = lsparse_string(name, optarg);
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
    lsprog_t *prog = lsparse_file(argv[i]);
    if (prog != NULL)
      lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
  }
}

static void lsprintloc(FILE *fp, const char *name, YYLTYPE *loc) {
  if (loc->first_line == loc->last_line) {
    if (loc->first_column == loc->last_column)
      lsprintf(fp, 0, "%s:%d.%d: ", name, loc->first_line, loc->first_column);
    else
      lsprintf(fp, 0, "%s:%d.%d-%d: ", name, loc->first_line, loc->first_column,
               loc->last_column);
  } else
    lsprintf(fp, 0, "%s:%d.%d-%d.%d: ", name, loc->first_line,
             loc->first_column, loc->last_line, loc->last_column);
}

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, const char *s) {
  lsprintloc(stderr, yyget_extra(scanner)->filename, yylloc);
  lsprintf(stderr, 0, "%s\n", s);
}

void lsprintf(FILE *fp, int indent, const char *fmt, ...) {
  if (indent == 0) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
  } else {
    va_list ap;
    va_start(ap, fmt);
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char buf[size + 1];
    va_start(ap, fmt);
    vsnprintf(buf, size + 1, fmt, ap);
    va_end(ap);
    const char *str = buf;
    while (*str != '\0') {
      const char *nl = strchr(str, '\n');
      if (nl == NULL) {
        fprintf(fp, "%s", str);
        break;
      }
      fwrite(str, 1, nl - str + 1, fp);
      for (int i = 0; i < indent; i++)
        fprintf(fp, "  ");
      str = nl + 1;
    }
  }
}
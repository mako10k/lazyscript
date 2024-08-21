#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lazyscript.h"
#include "lexer.h"
#include "parser.h"
#include <assert.h>
#include <gc.h>
#include <stdarg.h>

lsprog_t *lsparse_file(const char *filename) {
  assert(filename != NULL);
  yyscan_t scanner;
  yylex_init(&scanner);
  lsscan_t lsscan = {filename, NULL};
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror(filename);
    exit(1);
  }
  yyset_in(file, scanner);
  yyset_extra(&lsscan, scanner);
  int ret = yyparse(scanner);
  lsprog_t *prog = ret == 0 ? lsscan.prog : NULL;
  yylex_destroy(scanner);
  fclose(file);
  return prog;
}

int main(int argc, char **argv) {
#if DEBUG
  extern int yydebug;
  yydebug = 1;
#endif
  for (int i = 1; i < argc; i++) {
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
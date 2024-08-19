#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lazyscript.h"
#include "lexer.h"
#include "parser.h"
#include <gc.h>
#include <stdarg.h>

void lsparse_file(const char *filename) {
  yyscan_t scanner;
  yylex_init(&scanner);
  lsscan_t lsscan = {filename};
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror(filename);
    exit(1);
  }
  yyset_in(file, scanner);
  yyset_extra(&lsscan, scanner);
  yyparse(scanner);
  yylex_destroy(scanner);
  fclose(file);
}

int main(int argc, char **argv) {
#if DEBUG
  extern int yydebug;
  yydebug = 1;
#endif
  for (int i = 1; i < argc; i++)
    lsparse_file(argv[i]);
}

static void lsprintloc(FILE *fp, const char *name, YYLTYPE *loc) {
  if (loc->first_line == loc->last_line) {
    if (loc->first_column == loc->last_column)
      fprintf(fp, "%s:%d.%d: ", name, loc->first_line, loc->first_column);
    else
      fprintf(fp, "%s:%d.%d-%d: ", name, loc->first_line, loc->first_column,
              loc->last_column);
  } else
    fprintf(fp, "%s:%d.%d-%d.%d: ", name, loc->first_line, loc->first_column,
            loc->last_line, loc->last_column);
}

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, const char *s) {
  lsprintloc(stderr, yyget_extra(scanner)->filename, yylloc);
  fprintf(stderr, "%s\n", s);
}
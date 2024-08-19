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

void yyerror(YYLTYPE *yylloc, yyscan_t scanner, const char *s) {
  fprintf(stderr, "%s:%d.%d-%d.%d: E: %s\n", yyget_extra(scanner)->filename,
          yylloc->first_line, yylloc->first_column, yylloc->last_line,
          yylloc->last_column, s);
}
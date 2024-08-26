#include "io.h"
#include "loc.h"
#include <stdarg.h>
#include <string.h>

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

void yyerror(lsloc_t *loc, lsscan_t *scanner, const char *s) {
  (void)scanner;
  lsloc_print(stderr, *loc);
  lsprintf(stderr, 0, "%s\n", s);
}

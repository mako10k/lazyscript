#include "common/loc.h"
#include "common/io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

lsloc_t lsloc(const char* filename, int first_line, int first_column, int last_line,
              int last_column) {
  assert(filename != NULL);
  assert(first_line >= 1);
  assert(first_column >= 1);
  assert(last_line >= first_line);
  assert(last_line > first_line || last_column >= first_column);
  lsloc_t loc;
  loc.filename     = filename;
  loc.first_line   = first_line;
  loc.first_column = first_column;
  loc.last_line    = last_line;
  loc.last_column  = last_column;
  return loc;
}

void lsloc_print(FILE* fp, lsloc_t loc) {
  assert(fp != NULL);
  assert(loc.filename != NULL);
  lsprintf(fp, 0, "%s:%d.%d", loc.filename, loc.first_line, loc.first_column);
  if (loc.first_line == loc.last_line) {
    if (loc.first_column != loc.last_column)
      lsprintf(fp, 0, "-%d", loc.last_column);
  } else
    lsprintf(fp, 0, "-%d.%d", loc.last_line, loc.last_column);
  lsprintf(fp, 0, ": ");
}

void lsloc_update(lsloc_t* loc, const char* buf, int len) {
  assert(loc != NULL);
  assert(loc->filename != NULL);
  assert(buf != NULL);
  assert(len >= 0);
  int i             = 0;
  loc->first_line   = loc->last_line;
  loc->first_column = loc->last_column;
  while (i < len) {
    char* c = memchr(buf + i, '\n', len - i);
    if (c == NULL)
      break;
    loc->last_line++;
    loc->last_column = 1;
    i                = c - buf + 1;
  }
  loc->last_column += len - i;
}
#include "lazyscript.h"
#include "common/io.h"
#include "parser/parser.h"
#include "parser/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Local parse helper (duplicated from lazyscript.c)
static const char*     g_sugar_ns = NULL;

static const lsprog_t* lsparse_stream_local(const char* filename, FILE* in_str) {
  assert(in_str != NULL);
  yyscan_t yyscanner;
  yylex_init(&yyscanner);
  lsscan_t* lsscan = lsscan_new(filename);
  yyset_in(in_str, yyscanner);
  yyset_extra(lsscan, yyscanner);
  if (g_sugar_ns == NULL)
    g_sugar_ns = getenv("LAZYSCRIPT_SUGAR_NS");
  if (g_sugar_ns && g_sugar_ns[0])
    lsscan_set_sugar_ns(lsscan, g_sugar_ns);
  int             ret  = yyparse(yyscanner);
  const lsprog_t* prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
  yylex_destroy(yyscanner);
  return prog;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input.ls>\n", argv[0]);
    return 1;
  }
  const char* filename = argv[1];
  FILE*       fp       = fopen(filename, "r");
  if (!fp) {
    perror(filename);
    return 1;
  }
  const lsprog_t* prog = lsparse_stream_local(filename, fp);
  fclose(fp);
  if (!prog) {
    fprintf(stderr, "Parse error in %s\n", filename);
    return 2;
  }
  // Reproduce all source comments (best-effort) before the formatted output
  const lsarray_t* cs  = lsprog_get_comments(prog);
  int printed_any = 0;
  if (cs) {
    lssize_t n = lsarray_get_size(cs);
    const void* const* pv = lsarray_get(cs);
    for (lssize_t i = 0; i < n; i++) {
      const lscomment_t* c = (const lscomment_t*)pv[i];
      if (!c || !c->lc_text) continue;
      const char* s = lsstr_get_buf(c->lc_text);
      if (s && s[0]) {
        fputs(s, stdout);
        fputc('\n', stdout);
        printed_any = 1;
      }
    }
  }
  if (!printed_any) {
    // Fallback: scan the file and echo comment lines
    FILE* fp2 = fopen(filename, "r");
    if (fp2) {
      char*  line = NULL; size_t cap = 0; ssize_t len;
      while ((len = getline(&line, &cap, fp2)) != -1) {
        // Trim trailing '\n'
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        // Detect leading spaces + '#'
        size_t i = 0; while (line[i] == ' ' || line[i] == '\t') i++;
        if (line[i] == '#') {
          fputs(line + i, stdout); fputc('\n', stdout);
        }
        // TODO: Optionally handle block comments {- -} if needed
      }
      if (line) free(line);
      fclose(fp2);
    }
  }
  lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
  return 0;
}

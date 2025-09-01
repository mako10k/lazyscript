#include "lazyscript.h"
#include "common/io.h"
#include "parser/parser.h"
#include "parser/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include "misc/prog.h"

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

static void print_usage(const char* progname) {
  printf("Usage: %s [OPTIONS] [FILE ...]\n", progname);
  printf("Format LazyScript source. If no FILE is given, read from standard input.\n");
  printf("\nOptions:\n");
  printf("  -i, --in-place   format files in place (overwrite). When reading from stdin,\n");
  printf("                  this option is not allowed.\n");
  printf("  -h, --help       show this help and exit\n");
  printf("  -v, --version    print version and exit\n");
}

static int format_stream_to(const char* filename, FILE* in_fp, FILE* out_fp) {
  const lsprog_t* prog = lsparse_stream_local(filename, in_fp);
  if (!prog) {
    fprintf(stderr, "Parse error in %s\n", filename);
    return 2;
  }
  const char* dbg = getenv("LAZYFMT_DEBUG");
  if (dbg && dbg[0]) {
    const lsarray_t* cs = lsprog_get_comments(prog);
    lssize_t         n  = cs ? lsarray_get_size(cs) : 0;
    fprintf(stderr, "[lazyscript_format] captured comments: %ld\n", (long)n);
    if (dbg[0] == '2') {
      const void* const* pv = cs ? lsarray_get(cs) : NULL;
      for (lssize_t i = 0; i < n; i++) {
        const lscomment_t* c  = (const lscomment_t*)pv[i];
        int                ln = c ? c->lc_loc.first_line : -1;
        const char*        s  = (c && c->lc_text) ? lsstr_get_buf(c->lc_text) : "";
        fprintf(stderr, "  [c%ld] line=%d text='%s'\n", (long)i, ln, s);
      }
    }
  }
  // Enable comment weaving so comments are interleaved by source position
  lsfmt_set_comment_stream(lsprog_get_comments(prog));
  lsprog_print(out_fp, LSPREC_LOWEST, 0, prog);
  lsfmt_clear_comment_stream();
  return 0;
}

static int format_buffer_to(const char* name, const char* buf, size_t len, FILE* out_fp) {
  // Create a read stream from buffer
  FILE* in_fp = fmemopen((void*)buf, len, "r");
  if (!in_fp) {
    perror("fmemopen");
    return 1;
  }
  const lsprog_t* prog = lsparse_stream_local(name, in_fp);
  fclose(in_fp);
  if (!prog) {
    fprintf(stderr, "Parse error in %s\n", name);
    return 2;
  }
  const char* dbg = getenv("LAZYFMT_DEBUG");
  if (dbg && dbg[0]) {
    const lsarray_t* cs = lsprog_get_comments(prog);
    lssize_t         n  = cs ? lsarray_get_size(cs) : 0;
    fprintf(stderr, "[lazyscript_format] captured comments: %ld\n", (long)n);
    if (dbg[0] == '2') {
      const void* const* pv = cs ? lsarray_get(cs) : NULL;
      for (lssize_t i = 0; i < n; i++) {
        const lscomment_t* c  = (const lscomment_t*)pv[i];
        int                ln = c ? c->lc_loc.first_line : -1;
        const char*        s  = (c && c->lc_text) ? lsstr_get_buf(c->lc_text) : "";
        fprintf(stderr, "  [c%ld] line=%d text='%s'\n", (long)i, ln, s);
      }
    }
  }
  lsfmt_set_comment_stream(lsprog_get_comments(prog));
  lsprog_print(out_fp, LSPREC_LOWEST, 0, prog);
  lsfmt_clear_comment_stream();
  return 0;
}

static int format_file(const char* filename, int in_place) {
  if (strcmp(filename, "-") == 0 || strcmp(filename, "<stdin>") == 0)
    filename = "/dev/stdin";
  FILE* in_fp = NULL;
  if (strcmp(filename, "/dev/stdin") == 0) {
    if (in_place) {
      fprintf(stderr, "-i/--in-place cannot be used with standard input\n");
      return 1;
    }
    // Read all stdin to buffer
    size_t cap = 4096, len = 0;
    char*  buf = (char*)malloc(cap);
    if (!buf) {
      perror("malloc");
      return 1;
    }
    for (;;) {
      if (len == cap) {
        cap *= 2;
        char* nbuf = (char*)realloc(buf, cap);
        if (!nbuf) {
          free(buf);
          perror("realloc");
          return 1;
        }
        buf = nbuf;
      }
      size_t n = fread(buf + len, 1, cap - len, stdin);
      len += n;
      if (n == 0)
        break;
    }
    int rc = format_buffer_to("/dev/stdin", buf, len, stdout);
    free(buf);
    return rc;
  } else {
    in_fp = fopen(filename, "r");
    if (!in_fp) {
      perror(filename);
      return 1;
    }
  }

  int rc = 0;
  if (in_place && strcmp(filename, "/dev/stdin") != 0) {
    char tmpname[4096];
    snprintf(tmpname, sizeof(tmpname), "%s.tmp", filename);
    FILE* out_fp = fopen(tmpname, "w");
    if (!out_fp) {
      perror(tmpname);
      if (in_fp != stdin)
        fclose(in_fp);
      return 1;
    }
    rc = format_stream_to(filename, in_fp, out_fp);
    if (in_fp != stdin)
      fclose(in_fp);
    fclose(out_fp);
    if (rc == 0) {
      if (rename(tmpname, filename) != 0) {
        perror("rename");
        rc = 1;
      }
    } else {
      // keep tmp for inspection on error
    }
    return rc;
  }

  // default: write to stdout
  rc = format_stream_to(filename, in_fp, stdout);
  if (in_fp != stdin)
    fclose(in_fp);
  return rc;
}

int main(int argc, char** argv) {
  int           in_place   = 0;
  struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { "in-place", no_argument, NULL, 'i' },
    { NULL, 0, NULL, 0 },
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "hvi", longopts, NULL)) != -1) {
    switch (opt) {
    case 'h':
      print_usage(argv[0]);
      return 0;
    case 'v':
      printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      return 0;
    case 'i':
      in_place = 1;
      break;
    default:
      print_usage(argv[0]);
      return 1;
    }
  }

  int rc = 0;
  if (optind >= argc) {
    // No files: read from stdin, write to stdout
    rc = format_file("<stdin>", in_place);
    return rc;
  }

  for (int i = optind; i < argc; i++) {
    int frc = format_file(argv[i], in_place);
    if (frc != 0)
      rc = frc;
  }
  return rc;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "lazyscript.h"
#include "tools/cli_common.h"
#include "lstr/lstr.h"
#include "thunk/lsti.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"

static int cmp_files(const char* a, const char* b) {
  FILE* fa = fopen(a, "rb");
  FILE* fb = fopen(b, "rb");
  if (!fa || !fb) {
    if (fa)
      fclose(fa);
    if (fb)
      fclose(fb);
    return -1;
  }
  int           eq = 1;
  unsigned char ba[4096], bb[4096];
  for (;;) {
    size_t ra = fread(ba, 1, sizeof ba, fa);
    size_t rb = fread(bb, 1, sizeof bb, fb);
    if (ra != rb) {
      eq = 0;
      break;
    }
    if (ra == 0)
      break;
    if (memcmp(ba, bb, ra) != 0) {
      eq = 0;
      break;
    }
  }
  fclose(fa);
  fclose(fb);
  return eq ? 0 : 1;
}

static int dump_lstr_to_mem(const char* path, char** out_str, size_t* out_len) {
  if (!out_str || !out_len)
    return -1;
  *out_str             = NULL;
  *out_len             = 0;
  const lstr_prog_t* p = lstr_from_lsti_path(path);
  if (!p)
    return -2;
  FILE* ms = open_memstream(out_str, out_len);
  if (!ms)
    return -3;
  lstr_print(ms, 0, p);
  fflush(ms);
  fclose(ms);
  return 0;
}

int main(int argc, char** argv) {
  lscli_io_t io;
  lscli_io_init(&io, "./_tmp_test.lsti");
  const char* out_path = "./_tmp_rt.lsti";
  int cli_opt;
  while ((cli_opt = getopt_long(argc, argv, lscli_short_io_hv(), lscli_longopts_io_hv(), NULL)) != -1) {
    if (lscli_io_handle_opt(cli_opt, optarg, &io))
      continue;
    fprintf(stderr, "Try --help for usage.\n");
    return 1;
  }
  if (optind < argc)
    io.input_path = argv[optind++];
  if (io.output_path)
    out_path = io.output_path;
  if (io.want_help) {
  lscli_print_usage_basic(argv[0], /*has_input*/1, /*has_output*/1);
  printf("  (default output: ./_tmp_rt.lsti)\n");
    return 0;
  }
  if (io.want_version) {
    lscli_print_version();
    return 0;
  }
  const lstr_prog_t* prog = lstr_from_lsti_path(io.input_path);
  if (!prog) {
    fprintf(stderr, "fail: lstr_from_lsti_path(%s)\n", io.input_path);
    return 2;
  }
  if (lstr_validate(prog) != 0) {
    fprintf(stderr, "invalid LSTR\n");
    return 3;
  }
  lsthunk_t** roots = NULL;
  lssize_t    rootc = 0;
  lstenv_t*   env   = lstenv_new(NULL);
  if (lstr_materialize_to_thunks(prog, &roots, &rootc, env) != 0) {
    fprintf(stderr, "fail: to_thunks\n");
    return 4;
  }
  const char* out = out_path;
  FILE*       fp  = fopen(out, "wb");
  if (!fp) {
    perror("fopen");
    return 5;
  }
  lsti_write_opts_t opt = { .align_log2 = LSTI_ALIGN_8, .flags = 0 };
  int               rc  = lsti_write(fp, roots, rootc, &opt);
  fclose(fp);
  if (rc != 0) {
    fprintf(stderr, "lsti_write rc=%d\n", rc);
    return 6;
  }
  int diff = cmp_files(io.input_path, out);
  if (diff == 0) {
    printf("roundtrip: equal\n");
    return 0;
  }
  // Fall back to semantic equality via LSTR dump comparison
  char * s1 = NULL, *s2 = NULL;
  size_t n1 = 0, n2 = 0;
  if (dump_lstr_to_mem(io.input_path, &s1, &n1) == 0 && dump_lstr_to_mem(out, &s2, &n2) == 0) {
    int semeq = (n1 == n2 && s1 && s2 && memcmp(s1, s2, n1) == 0) ? 1 : 0;
    free(s1);
    free(s2);
    if (semeq) {
      printf("roundtrip: semantically-equal (bytes differ)\n");
      return 0;
    }
  }
  printf("roundtrip: different\n");
  return 1;
}

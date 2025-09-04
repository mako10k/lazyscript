#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "lazyscript.h"
#include "lstr/lstr.h"
#include "tools/cli_common.h"
#include "tools/lstr_prog_common.h"

int main(int argc, char** argv) {
  lscli_io_t io;
  lscli_io_init(&io, "./_tmp_test.lsti");
  int opt;
  while ((opt = getopt_long(argc, argv, lscli_short_io_hv(), lscli_longopts_io_hv(), NULL)) != -1) {
    if (lscli_io_handle_opt(opt, optarg, &io))
      continue;
    fprintf(stderr, "Try --help for usage.\n");
    return 1;
  }
  if (optind < argc)
    io.input_path = argv[optind++];
  if (lscli_maybe_handle_hv(argv[0], &io, /*has_input*/1, /*has_output*/1))
    return 0;
  FILE* outfp = lscli_open_output_or_stdout(io.output_path);
  if (!outfp)
    return 1;
  const lstr_prog_t* p = NULL;
  {
    int rc = lsprog_load_validate(io.input_path, &p);
    if (rc)
      return rc;
  }
  lstr_print(outfp, 0, p);
  lscli_close_if_not_stdout(outfp);
  return 0;
}

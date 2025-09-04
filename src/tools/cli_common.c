#include "tools/cli_common.h"
#include <string.h>
#include "lazyscript.h"

static const struct option g_longopts_io_hv[] = {
  { "input", required_argument, NULL, 'i' },
  { "output", required_argument, NULL, 'o' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 },
};
static const char* g_short_io_hv = "i:o:hv";

static const struct option g_longopts_io_r_hv[] = {
  { "input", required_argument, NULL, 'i' },
  { "output", required_argument, NULL, 'o' },
  { "root", required_argument, NULL, 'r' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 },
};
static const char* g_short_io_r_hv = "i:o:r:hv";

void lscli_io_init(lscli_io_t* io, const char* default_input) {
  if (!io)
    return;
  io->input_path   = default_input;
  io->output_path  = NULL;
  io->want_help    = 0;
  io->want_version = 0;
}

int lscli_io_handle_opt(int opt, const char* optarg, lscli_io_t* io) {
  if (!io)
    return 0;
  switch (opt) {
  case 'i':
    io->input_path = optarg;
    return 1;
  case 'o':
    io->output_path = optarg;
    return 1;
  case 'h':
    io->want_help = 1;
    return 1;
  case 'v':
    io->want_version = 1;
    return 1;
  default:
    return 0;
  }
}

void lscli_print_version(void) {
  printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
}

void lscli_print_usage_basic(const char* argv0, int has_input, int has_output) {
  printf("Usage: %s ", argv0);
  if (has_input)
    printf("[-i FILE] ");
  if (has_output)
    printf("[-o FILE] ");
  printf("[-h] [-v]\n");
  if (has_input)
    printf("  -i, --input FILE    input file\n");
  if (has_output)
    printf("  -o, --output FILE   output file (or '-' for stdout)\n");
  printf("  -h, --help          show this help and exit\n");
  printf("  -v, --version       print version and exit\n");
}

FILE* lscli_open_output_or_stdout(const char* output_path) {
  if (!output_path || strcmp(output_path, "-") == 0)
    return stdout;
  FILE* fp = fopen(output_path, "w");
  if (!fp)
    perror(output_path);
  return fp;
}

int lscli_close_if_not_stdout(FILE* fp) {
  if (fp && fp != stdout)
    return fclose(fp);
  return 0;
}

const struct option* lscli_longopts_io_hv(void) { return g_longopts_io_hv; }
const char*          lscli_short_io_hv(void) { return g_short_io_hv; }
const struct option* lscli_longopts_io_r_hv(void) { return g_longopts_io_r_hv; }
const char*          lscli_short_io_r_hv(void) { return g_short_io_r_hv; }

int lscli_maybe_handle_hv(const char* argv0, lscli_io_t* io, int has_input, int has_output) {
  if (!io)
    return 0;
  if (io->want_help) {
    lscli_print_usage_basic(argv0, has_input, has_output);
    return 1;
  }
  if (io->want_version) {
    lscli_print_version();
    return 1;
  }
  return 0;
}

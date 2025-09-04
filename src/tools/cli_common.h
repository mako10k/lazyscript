#ifndef LS_TOOLS_CLI_COMMON_H
#define LS_TOOLS_CLI_COMMON_H

#include <stdio.h>
#include <getopt.h>

typedef struct lscli_io_s {
  const char* input_path;   // default may be NULL
  const char* output_path;  // default NULL means stdout
  int         want_help;    // set when -h/--help seen
  int         want_version; // set when -v/--version seen
} lscli_io_t;

// Initialize IO options with default input path (may be NULL)
void lscli_io_init(lscli_io_t* io, const char* default_input);

// Handle common getopt option: returns 1 if handled ('i','o','h','v'), else 0
int lscli_io_handle_opt(int opt, const char* optarg, lscli_io_t* io);

// Print standard version line: "<PACKAGE_NAME> <PACKAGE_VERSION>"
void lscli_print_version(void);

// Print a basic usage header and standard options (-i/-o if present, -h/-v always)
// Tool can pass has_input/has_output to control listing of -i/-o lines.
void lscli_print_usage_basic(const char* argv0, int has_input, int has_output);

// Open output file or return stdout when output_path is NULL or "-".
// Returns NULL on error and prints perror(output_path) when applicable.
FILE* lscli_open_output_or_stdout(const char* output_path);

// Close file if it's not stdout; return 0 on success.
int lscli_close_if_not_stdout(FILE* fp);

// Reusable getopt_long option tables and short option strings
// Patterns:
//  - io+hv: -i/-o plus -h/-v
//  - io+r+hv: -i/-o/-r plus -h/-v
const struct option* lscli_longopts_io_hv(void);
const char*          lscli_short_io_hv(void);
const struct option* lscli_longopts_io_r_hv(void);
const char*          lscli_short_io_r_hv(void);

// If -h/--help or -v/--version were requested, print and return 1 (caller should exit 0).
// Otherwise return 0 and let the tool continue.
int lscli_maybe_handle_hv(const char* argv0, lscli_io_t* io, int has_input, int has_output);

#endif // LS_TOOLS_CLI_COMMON_H

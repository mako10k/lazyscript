#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "lazyscript.h"
#include "lstr/lstr.h"

int main(int argc, char** argv) {
  const char* input_path = "./_tmp_test.lsti";
  const char* output_path = NULL; // stdout
  int opt;
  struct option longopts[] = {
    { "input", required_argument, NULL, 'i' },
    { "output", required_argument, NULL, 'o' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 },
  };
  while ((opt = getopt_long(argc, argv, "i:o:hv", longopts, NULL)) != -1) {
    switch (opt) {
      case 'i': input_path = optarg; break;
      case 'o': output_path = optarg; break;
      case 'h':
        printf("Usage: %s [-i FILE] [-o FILE]\n", argv[0]);
        printf("  -i, --input FILE    LSTI input file (default: ./_tmp_test.lsti)\n");
        printf("  -o, --output FILE   write dump to FILE instead of stdout\n");
        printf("  -h, --help          show this help and exit\n");
        printf("  -v, --version       print version and exit\n");
        return 0;
      case 'v':
        printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        return 0;
      default:
        fprintf(stderr, "Try --help for usage.\n");
        return 1;
    }
  }
  if (optind < argc) input_path = argv[optind++];
  FILE* outfp = stdout;
  if (output_path && strcmp(output_path, "-") != 0) {
    outfp = fopen(output_path, "w"); if (!outfp) { perror(output_path); return 1; }
  }
  const lstr_prog_t* p = lstr_from_lsti_path(input_path);
  if (!p) {
    fprintf(stderr, "failed: lstr_from_lsti_path(%s)\n", input_path);
    return 1;
  }
  if (lstr_validate(p) != 0) {
    fprintf(stderr, "invalid LSTR\n");
    return 2;
  }
  lstr_print(outfp, 0, p);
  if (outfp && outfp != stdout) fclose(outfp);
  return 0;
}

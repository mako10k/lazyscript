#include <stdio.h>
#include <stdlib.h>
#include "lstr/lstr.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include "lazyscript.h"
#include "tools/cli_common.h"
#include "tools/llvm_emit_common.h"
#include "tools/lstr_prog_common.h"

// Minimal LSTR -> LLVM IR (text) emitter
// - Input: LSTI image path (default: ./_tmp_test.lsti)
// - Output: LLVM IR textual module to stdout
// Behavior:
//   If root[0] is an INT constant, emit main returning that value.
//   If root[0] is ALGE(.Ok <INT>), return that INT.
//   If root[0] is a STR constant, emit a call to puts and return 0.

// Note: current stage avoids generating external calls for REF/APPL.
// They are treated as opaque and result in a trivial main returning 0.

int main(int argc, char** argv) {
  long       root_index = 0;
  lscli_io_t io;
  lscli_io_init(&io, "./_tmp_test.lsti");
  // CLI: -i/--input, -o/--output, -r/--root, -h/--help, -v/--version
  int opt;
  while ((opt = getopt_long(argc, argv, lscli_short_io_r_hv(), lscli_longopts_io_r_hv(), NULL)) != -1) {
    if (lscli_io_handle_opt(opt, optarg, &io))
      continue;
    switch (opt) {
    case 'r': {
      char* end = NULL;
      long  v   = strtol(optarg, &end, 10);
      if (end && *end == '\0' && v >= 0)
        root_index = v;
      else {
        fprintf(stderr, "invalid --root value: %s\n", optarg);
        return 1;
      }
      break;
    }
    default:
      fprintf(stderr, "Try --help for usage.\n");
      return 1;
    }
  }
  // Backward-compat positional args: [INPUT [ROOT_INDEX]] if provided
  if (optind < argc) {
    io.input_path = argv[optind++];
  }
  if (optind < argc) {
    char* end = NULL;
    long  v   = strtol(argv[optind++], &end, 10);
    if (end && *end == '\0' && v >= 0)
      root_index = v;
  }
  if (lscli_maybe_handle_hv(argv[0], &io, /*has_input*/1, /*has_output*/1)) {
    if (io.want_help)
      printf("  -r, --root INDEX    root index to inspect (default: 0)\n");
    return 0;
  }

  FILE* outfp = lscli_open_output_or_stdout(io.output_path);
  if (!outfp)
    return 1;
  const lstr_prog_t* p = NULL;
  {
    int rc = lsprog_load_validate(io.input_path, &p);
    if (rc)
      return rc;
  }
  lstenv_t*   env   = NULL;
  lsthunk_t** roots = NULL;
  lssize_t    rootc = 0;
  {
    int rc = lsprog_materialize_roots(p, &roots, &rootc, &env);
    if (rc)
      return rc;
  }
  lsemit_ir_prelude(outfp);
  if (rootc > 0 && roots && root_index < rootc && roots[root_index]) {
    lsthunk_t* r0 = roots[root_index];
    switch (lsthunk_get_type(r0)) {
    case LSTTYPE_INT: {
      lsemit_emit_main_ret_from_int(outfp, r0);
      break;
    }
    case LSTTYPE_ALGE: {
      // 純ADT方針: タグの解釈を行わず、現段階ではコード生成を行わない
      // 将来的にタグ付きユニオン構築/比較の表現を追加する
      lsemit_ir_main_ret_i32(outfp, 0);
      break;
    }
    case LSTTYPE_SYMBOL: {
      lsemit_emit_symbol_global_and_main(outfp, r0);
      break;
    }
    case LSTTYPE_STR: {
      lsemit_emit_main_puts_from_str(outfp, r0);
      break;
    }
    case LSTTYPE_REF: {
      // 現段階ではREFは外部関数にマップせず、薄いラッパとして0を返す
      lsemit_ir_main_ret_i32(outfp, 0);
      break;
    }
    case LSTTYPE_APPL: {
      // 現段階ではAPPLも呼出しを生成せず、薄いラッパ（0を返す）
      lsemit_ir_main_ret_i32(outfp, 0);
      break;
    }
    default:
      lsemit_ir_main_ret_i32(outfp, 0);
    }
  } else {
    lsemit_ir_main_ret_i32(outfp, 0);
  }
  lscli_close_if_not_stdout(outfp);
  return 0;
}

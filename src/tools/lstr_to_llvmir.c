#include <stdio.h>
#include <stdlib.h>
#include "lstr/lstr.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include "lazyscript.h"

// Minimal LSTR -> LLVM IR (text) emitter
// - Input: LSTI image path (default: ./_tmp_test.lsti)
// - Output: LLVM IR textual module to stdout
// Behavior:
//   If root[0] is an INT constant, emit main returning that value.
//   If root[0] is ALGE(.Ok <INT>), return that INT.
//   If root[0] is a STR constant, emit a call to puts and return 0.

static void emit_ir_prelude(FILE* out) {
  fprintf(out, "; ModuleID = 'lazyscript'\n");
  fprintf(out, "source_filename = \"lazyscript\"\n\n");
}

static void emit_ir_main_ret_i32(FILE* out, int v) {
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  ret i32 %d\n", v);
  fprintf(out, "}\n");
}

static void emit_ir_decl_puts(FILE* out) {
  // Opaque pointers friendly
  fprintf(out, "declare i32 @puts(ptr)\n\n");
}

static void emit_ir_global_str(FILE* out, const char* cstr, size_t n) {
  // Emit a private constant: @.str = private unnamed_addr constant [N x i8] c"...\\00"
  fprintf(out, "@.str = private unnamed_addr constant [%zu x i8] c\"", n + 1);
  for (size_t i = 0; i < n; i++) {
    unsigned char ch = (unsigned char)cstr[i];
    switch (ch) {
    case '\\':
      fprintf(out, "\\\\");
      break;
    case '"':
      fprintf(out, "\\\"");
      break;
    case '\n':
      fprintf(out, "\\0A");
      break;
    case '\r':
      fprintf(out, "\\0D");
      break;
    case '\t':
      fprintf(out, "\\09");
      break;
    default:
      if (ch >= 32 && ch <= 126) {
        fputc(ch, out);
      } else {
        fprintf(out, "\\%02X", ch);
      }
    }
  }
  fprintf(out, "\\00\"\n\n");
}

static void emit_ir_global_named_str(FILE* out, const char* gname, const char* cstr, size_t n) {
  // Emit named private constant: @gname = private unnamed_addr constant [N x i8] c"...\00"
  fprintf(out, "@%s = private unnamed_addr constant [%zu x i8] c\"", gname, n + 1);
  for (size_t i = 0; i < n; i++) {
    unsigned char ch = (unsigned char)cstr[i];
    switch (ch) {
    case '\\':
      fprintf(out, "\\\\");
      break;
    case '"':
      fprintf(out, "\\\"");
      break;
    case '\n':
      fprintf(out, "\\0A");
      break;
    case '\r':
      fprintf(out, "\\0D");
      break;
    case '\t':
      fprintf(out, "\\09");
      break;
    default:
      if (ch >= 32 && ch <= 126) {
        fputc(ch, out);
      } else {
        fprintf(out, "\\%02X", ch);
      }
    }
  }
  fprintf(out, "\\00\"\n\n");
}

static void emit_ir_main_puts(FILE* out, size_t n) {
  // gep to the head of @.str and call puts
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  %%0 = getelementptr inbounds ([%zu x i8], ptr @.str, i64 0, i64 0)\n", n + 1);
  fprintf(out, "  %%1 = call i32 @puts(ptr %%0)\n");
  fprintf(out, "  ret i32 0\n");
  fprintf(out, "}\n");
}

static void mangle_ref_name(const lsstr_t* name, char* out, size_t outsz) {
  // Very simple mangling: prefix "fn_" and map non [A-Za-z0-9_] to '_'
  if (!out || outsz == 0)
    return;
  size_t      pos = 0;
  const char* s   = lsstr_get_buf(name);
  lssize_t    n   = lsstr_get_len(name);
  if (outsz > 3) {
    out[pos++] = 'f';
    out[pos++] = 'n';
    out[pos++] = '_';
  }
  for (lssize_t i = 0; i < n && pos + 1 < outsz; i++) {
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c == '_')
      out[pos++] = (char)c;
    else
      out[pos++] = '_';
  }
  out[pos] = '\0';
}

// Note: current stage avoids generating external calls for REF/APPL.
// They are treated as opaque and result in a trivial main returning 0.

static void mangle_sym_ir_label(const char* s, size_t n, char* out, size_t outsz) {
  // Build a safe label for IR identifiers: map non [A-Za-z0-9_.] to '_'
  if (!out || outsz == 0)
    return;
  size_t pos = 0;
  for (size_t i = 0; i < n && pos + 1 < outsz; i++) {
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c == '_' || c == '.')
      out[pos++] = (char)c;
    else
      out[pos++] = '_';
  }
  out[pos] = '\0';
}

static void emit_ir_main_bind_sym_and_ret0(FILE* out, const char* reg, const char* gname,
                                           size_t n) {
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  %%%s = getelementptr inbounds ([%zu x i8], ptr @%s, i64 0, i64 0)\n", reg, n + 1,
          gname);
  fprintf(out, "  ret i32 0\n");
  fprintf(out, "}\n");
}

int main(int argc, char** argv) {
  const char* input_path  = "./_tmp_test.lsti";
  const char* output_path = NULL; // stdout by default
  long        root_index  = 0;

  // CLI: -i/--input, -o/--output, -r/--root, -h/--help, -v/--version
  int           opt;
  struct option longopts[] = {
    { "input", required_argument, NULL, 'i' }, { "output", required_argument, NULL, 'o' },
    { "root", required_argument, NULL, 'r' },  { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },     { NULL, 0, NULL, 0 },
  };
  while ((opt = getopt_long(argc, argv, "i:o:r:hv", longopts, NULL)) != -1) {
    switch (opt) {
    case 'i':
      input_path = optarg;
      break;
    case 'o':
      output_path = optarg;
      break;
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
    case 'h':
      printf("Usage: %s [-i FILE] [-r INDEX] [-o FILE]\n", argv[0]);
      printf("  -i, --input FILE    LSTI input file (default: ./_tmp_test.lsti)\n");
      printf("  -r, --root INDEX    root index to inspect (default: 0)\n");
      printf("  -o, --output FILE   write LLVM IR to FILE instead of stdout\n");
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
  // Backward-compat positional args: [INPUT [ROOT_INDEX]] if provided
  if (optind < argc) {
    input_path = argv[optind++];
  }
  if (optind < argc) {
    char* end = NULL;
    long  v   = strtol(argv[optind++], &end, 10);
    if (end && *end == '\0' && v >= 0)
      root_index = v;
  }

  FILE* outfp = stdout;
  if (output_path && strcmp(output_path, "-") != 0) {
    outfp = fopen(output_path, "w");
    if (!outfp) {
      perror(output_path);
      return 1;
    }
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
  // Materialize minimal roots to inspect constants
  lstenv_t*   env   = lstenv_new(NULL);
  lsthunk_t** roots = NULL;
  lssize_t    rootc = 0;
  if (lstr_materialize_to_thunks(p, &roots, &rootc, env) != 0) {
    fprintf(stderr, "materialize failed\n");
    return 3;
  }
  emit_ir_prelude(outfp);
  if (rootc > 0 && roots && root_index < rootc && roots[root_index]) {
    lsthunk_t* r0 = roots[root_index];
    switch (lsthunk_get_type(r0)) {
    case LSTTYPE_INT: {
      const lsint_t* iv   = lsthunk_get_int(r0);
      int            retv = iv ? lsint_get(iv) : 0;
      emit_ir_main_ret_i32(outfp, retv);
      break;
    }
    case LSTTYPE_ALGE: {
      // 純ADT方針: タグの解釈を行わず、現段階ではコード生成を行わない
      // 将来的にタグ付きユニオン構築/比較の表現を追加する
      emit_ir_main_ret_i32(outfp, 0);
      break;
    }
    case LSTTYPE_SYMBOL: {
      const lsstr_t* sv = lsthunk_get_symbol(r0);
      if (sv) {
        const char* s = lsstr_get_buf(sv);
        size_t      n = (size_t)lsstr_get_len(sv);
        // Strip leading '.' from textual content when emitting the string value
        const char* text  = s;
        size_t      textn = n;
        if (textn > 0 && text[0] == '.') {
          text++;
          textn--;
        }
        // Build IR-friendly label base
        char labelbuf[256];
        mangle_sym_ir_label(text, textn, labelbuf, sizeof labelbuf);
        // Global name and register name
        char gname[300];
        snprintf(gname, sizeof gname, "sym.%s", labelbuf);
        char reg[300];
        snprintf(reg, sizeof reg, "sym.%s", labelbuf);
        // Emit global for symbol text
        emit_ir_global_named_str(outfp, gname, text, textn);
        // Bind pointer in main and return 0
        emit_ir_main_bind_sym_and_ret0(outfp, reg, gname, textn);
        break;
      }
      emit_ir_main_ret_i32(outfp, 0);
      break;
    }
    case LSTTYPE_STR: {
      const lsstr_t* sv = lsthunk_get_str(r0);
      const char*    s  = sv ? lsstr_get_buf(sv) : "";
      size_t         n  = sv ? (size_t)lsstr_get_len(sv) : 0;
      emit_ir_decl_puts(outfp);
      emit_ir_global_str(outfp, s, n);
      emit_ir_main_puts(outfp, n);
      break;
    }
    case LSTTYPE_REF: {
      // 現段階ではREFは外部関数にマップせず、薄いラッパとして0を返す
      emit_ir_main_ret_i32(outfp, 0);
      break;
    }
    case LSTTYPE_APPL: {
      // 現段階ではAPPLも呼出しを生成せず、薄いラッパ（0を返す）
      emit_ir_main_ret_i32(outfp, 0);
      break;
    }
    default:
      emit_ir_main_ret_i32(outfp, 0);
    }
  } else {
    emit_ir_main_ret_i32(outfp, 0);
  }
  if (outfp && outfp != stdout)
    fclose(outfp);
  return 0;
}

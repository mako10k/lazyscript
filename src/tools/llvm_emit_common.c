#include "tools/llvm_emit_common.h"
#include <ctype.h>

void lsemit_ir_prelude(FILE* out) {
  fprintf(out, "; ModuleID = 'lazyscript'\n");
  fprintf(out, "source_filename = \"lazyscript\"\n\n");
}

void lsemit_ir_main_ret_i32(FILE* out, int v) {
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  ret i32 %d\n", v);
  fprintf(out, "}\n");
}

void lsemit_ir_decl_puts(FILE* out) {
  fprintf(out, "declare i32 @puts(ptr)\n\n");
}

void lsemit_ir_global_str(FILE* out, const char* cstr, size_t n) {
  // delegate to named variant with default name
  lsemit_ir_global_named_str(out, ".str", cstr, n);
}

void lsemit_ir_global_named_str(FILE* out, const char* gname, const char* cstr, size_t n) {
  // shared payload emission
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

void lsemit_ir_main_puts(FILE* out, size_t n) {
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  %%0 = getelementptr inbounds ([%zu x i8], ptr @.str, i64 0, i64 0)\n", n + 1);
  fprintf(out, "  %%1 = call i32 @puts(ptr %%0)\n");
  fprintf(out, "  ret i32 0\n");
  fprintf(out, "}\n");
}

void lsemit_mangle_ref_name(const lsstr_t* name, char* out, size_t outsz) {
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

void lsemit_mangle_sym_ir_label(const char* s, size_t n, char* out, size_t outsz) {
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

void lsemit_ir_main_bind_sym_and_ret0(FILE* out, const char* reg, const char* gname, size_t n) {
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  %%%s = getelementptr inbounds ([%zu x i8], ptr @%s, i64 0, i64 0)\n", reg, n + 1, gname);
  fprintf(out, "  ret i32 0\n");
  fprintf(out, "}\n");
}

void lsemit_emit_main_ret_from_int(FILE* out, const lsthunk_t* t) {
  const lsint_t* iv   = t ? lsthunk_get_int(t) : NULL;
  int            retv = iv ? lsint_get(iv) : 0;
  lsemit_ir_main_ret_i32(out, retv);
}

void lsemit_emit_main_puts_from_str(FILE* out, const lsthunk_t* t) {
  const lsstr_t* sv = t ? lsthunk_get_str(t) : NULL;
  const char*    s  = sv ? lsstr_get_buf(sv) : "";
  size_t         n  = sv ? (size_t)lsstr_get_len(sv) : 0;
  lsemit_ir_decl_puts(out);
  lsemit_ir_global_str(out, s, n);
  lsemit_ir_main_puts(out, n);
}

void lsemit_emit_symbol_global_and_main(FILE* out, const lsthunk_t* t) {
  const lsstr_t* sv = t ? lsthunk_get_symbol(t) : NULL;
  if (!sv) {
    lsemit_ir_main_ret_i32(out, 0);
    return;
  }
  const char* s = lsstr_get_buf(sv);
  size_t      n = (size_t)lsstr_get_len(sv);
  const char* text  = s;
  size_t      textn = n;
  if (textn > 0 && text[0] == '.') {
    text++;
    textn--;
  }
  char labelbuf[256];
  lsemit_mangle_sym_ir_label(text, textn, labelbuf, sizeof labelbuf);
  char gname[300];
  snprintf(gname, sizeof gname, "sym.%s", labelbuf);
  char reg[300];
  snprintf(reg, sizeof reg, "sym.%s", labelbuf);
  lsemit_ir_global_named_str(out, gname, text, textn);
  lsemit_ir_main_bind_sym_and_ret0(out, reg, gname, textn);
}

// --- External call emission helpers (shared) ---
void lsemit_ir_decl_ext_i32(FILE* out, const char* fname, int argc) {
  fprintf(out, "declare i32 @%s(", fname);
  for (int i = 0; i < argc; i++) {
    if (i)
      fputs(", ", out);
    fputs("i32", out);
  }
  fputs(")\n\n", out);
}

void lsemit_ir_main_call_ext_ret_i32(FILE* out, const char* fname, int argc, const int* args) {
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fputs("  %0 = call i32 @", out);
  fputs(fname, out);
  fputc('(', out);
  for (int i = 0; i < argc; i++) {
    if (i)
      fputs(", ", out);
    if (args)
      fprintf(out, "i32 %d", args[i]);
    else
      fputs("i32 0", out);
  }
  fputs(")\n", out);
  fputs("  ret i32 %0\n", out);
  fputs("}\n", out);
}

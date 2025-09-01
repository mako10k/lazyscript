#include <stdio.h>
#include <stdlib.h>
#include "lstr/lstr.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include <ctype.h>
#include <string.h>

// Minimal LSTR -> LLVM IR (text) emitter
// - Input: LSTI image path (default: ./_tmp_test.lsti)
// - Output: LLVM IR textual module to stdout
// Behavior:
//   If root[0] is an INT constant, emit main returning that value.
//   If root[0] is ALGE(.Ok <INT>), return that INT.
//   If root[0] is a STR constant, emit a call to puts and return 0.

static void emit_ir_prelude(FILE* out){
  fprintf(out, "; ModuleID = 'lazyscript'\n");
  fprintf(out, "source_filename = \"lazyscript\"\n\n");
}

static void emit_ir_main_ret_i32(FILE* out, int v){
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  ret i32 %d\n", v);
  fprintf(out, "}\n");
}

static void emit_ir_decl_puts(FILE* out){
  // Opaque pointers friendly
  fprintf(out, "declare i32 @puts(ptr)\n\n");
}

static void emit_ir_global_str(FILE* out, const char* cstr, size_t n){
  // Emit a private constant: @.str = private unnamed_addr constant [N x i8] c"...\\00"
  fprintf(out, "@.str = private unnamed_addr constant [%zu x i8] c\"", n + 1);
  for(size_t i=0;i<n;i++){
    unsigned char ch = (unsigned char)cstr[i];
    switch(ch){
      case '\\': fprintf(out, "\\\\"); break;
      case '"': fprintf(out, "\\\""); break;
      case '\n': fprintf(out, "\\0A"); break;
      case '\r': fprintf(out, "\\0D"); break;
      case '\t': fprintf(out, "\\09"); break;
      default:
        if (ch >= 32 && ch <= 126) { fputc(ch, out); }
        else { fprintf(out, "\\%02X", ch); }
    }
  }
  fprintf(out, "\\00\"\n\n");
}

static void emit_ir_main_puts(FILE* out, size_t n){
  // gep to the head of @.str and call puts
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fprintf(out, "  %%0 = getelementptr inbounds ([%zu x i8], ptr @.str, i64 0, i64 0)\n", n + 1);
  fprintf(out, "  %%1 = call i32 @puts(ptr %%0)\n");
  fprintf(out, "  ret i32 0\n");
  fprintf(out, "}\n");
}

static void mangle_ref_name(const lsstr_t* name, char* out, size_t outsz){
  // Very simple mangling: prefix "fn_" and map non [A-Za-z0-9_] to '_'
  if (!out || outsz==0) return;
  size_t pos = 0;
  const char* s = lsstr_get_buf(name);
  lssize_t n = lsstr_get_len(name);
  if (outsz > 3){ out[pos++]='f'; out[pos++]='n'; out[pos++]='_'; }
  for(lssize_t i=0;i<n && pos+1<outsz;i++){
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c=='_') out[pos++]=(char)c; else out[pos++]='_';
  }
  out[pos]='\0';
}

static void emit_ir_decl_ext(FILE* out, const char* fname, int argc){
  fprintf(out, "declare i32 @%s(", fname);
  for(int i=0;i<argc;i++){
    if(i) fputs(", ", out);
    fputs("i32", out);
  }
  fputs(")\n\n", out);
}

static void emit_ir_main_call_ext_i32(FILE* out, const char* fname, int argc, const int* args){
  fprintf(out, "define i32 @main() {\n");
  fprintf(out, "entry:\n");
  fputs("  %0 = call i32 @", out); fputs(fname, out); fputc('(', out);
  for(int i=0;i<argc;i++){
    if(i) fputs(", ", out);
    fprintf(out, "i32 %d", args[i]);
  }
  fputs(")\n", out);
  fputs("  ret i32 %0\n", out);
  fputs("}\n", out);
}

int main(int argc, char** argv){
  const char* path = (argc > 1) ? argv[1] : "./_tmp_test.lsti";
  long root_index = 0;
  if (argc > 2) {
    char *end=NULL; long v = strtol(argv[2], &end, 10);
    if (end && *end=='\0' && v >= 0) root_index = v;
  }
  const lstr_prog_t* p = lstr_from_lsti_path(path);
  if(!p){
    fprintf(stderr, "failed: lstr_from_lsti_path(%s)\n", path);
    return 1;
  }
  if(lstr_validate(p)!=0){
    fprintf(stderr, "invalid LSTR\n");
    return 2;
  }
  // Materialize minimal roots to inspect constants
  lstenv_t* env = lstenv_new(NULL);
  lsthunk_t** roots = NULL; lssize_t rootc = 0;
  if(lstr_materialize_to_thunks(p, &roots, &rootc, env)!=0){
    fprintf(stderr, "materialize failed\n");
    return 3;
  }
  emit_ir_prelude(stdout);
  if(rootc > 0 && roots && root_index < rootc && roots[root_index]){
    lsthunk_t* r0 = roots[root_index];
    switch(lsthunk_get_type(r0)){
      case LSTTYPE_INT: {
        const lsint_t* iv = lsthunk_get_int(r0);
        int retv = iv ? lsint_get(iv) : 0;
        emit_ir_main_ret_i32(stdout, retv);
        break;
      }
      case LSTTYPE_ALGE: {
        const lsstr_t* c = lsthunk_get_constr(r0);
        if (c && lsstrcmp(c, lsstr_cstr(".Ok")) == 0 && lsthunk_get_argc(r0) == 1) {
          lsthunk_t* const* args = lsthunk_get_args(r0);
          if (args && args[0] && lsthunk_get_type(args[0]) == LSTTYPE_INT){
            const lsint_t* iv = lsthunk_get_int(args[0]);
            int retv = iv ? lsint_get(iv) : 0;
            emit_ir_main_ret_i32(stdout, retv);
            break;
          }
        }
        // Fallback
        emit_ir_main_ret_i32(stdout, 0);
        break;
      }
      case LSTTYPE_STR: {
        const lsstr_t* sv = lsthunk_get_str(r0);
        const char* s = sv ? lsstr_get_buf(sv) : "";
        size_t n = sv ? (size_t)lsstr_get_len(sv) : 0;
        emit_ir_decl_puts(stdout);
        emit_ir_global_str(stdout, s, n);
        emit_ir_main_puts(stdout, n);
        break;
      }
      case LSTTYPE_REF: {
        const lsstr_t* nm = lsthunk_get_ref_name(r0);
        if (nm){
          char m[256]; mangle_ref_name(nm, m, sizeof m);
          emit_ir_decl_ext(stdout, m, 0);
          emit_ir_main_call_ext_i32(stdout, m, 0, NULL);
          break;
        }
        emit_ir_main_ret_i32(stdout, 0);
        break;
      }
      case LSTTYPE_APPL: {
        lsthunk_t* fn = lsthunk_get_appl_func(r0);
        if (fn && lsthunk_get_type(fn)==LSTTYPE_REF){
          const lsstr_t* nm = lsthunk_get_ref_name(fn);
          lssize_t ac = lsthunk_get_argc(r0);
          lsthunk_t* const* av = lsthunk_get_args(r0);
          // Only support all-int arguments for now
          int ok=1; for(lssize_t i=0;i<ac;i++){ if(!(av && av[i] && lsthunk_get_type(av[i])==LSTTYPE_INT)){ ok=0; break; } }
          if (ok && nm){
            int bufc = (ac>0 && ac<16)? (int)ac : 0; // cap for stack array safety
            int argv[16]; for(int i=0;i<bufc;i++){ const lsint_t* iv = lsthunk_get_int(av[i]); argv[i] = iv ? lsint_get(iv) : 0; }
            char m[256]; mangle_ref_name(nm, m, sizeof m);
            emit_ir_decl_ext(stdout, m, bufc);
            emit_ir_main_call_ext_i32(stdout, m, bufc, argv);
            break;
          }
        }
        emit_ir_main_ret_i32(stdout, 0);
        break;
      }
      default:
        emit_ir_main_ret_i32(stdout, 0);
    }
  } else {
    emit_ir_main_ret_i32(stdout, 0);
  }
  return 0;
}

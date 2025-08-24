#include "runtime/effects.h"
#include "runtime/unit.h"
#include "runtime/modules.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/error.h"
#include <string.h>
#include <stdlib.h>

// Provided by host parser
const lsprog_t* lsparse_file_nullable(const char* filename);
const lsprog_t* lsparse_file(const char* filename);

static const lsprog_t* ls_require_resolve(const char* path) {
  const lsprog_t* prog = lsparse_file_nullable(path);
  if (prog) return prog;
  const char* spath = getenv("LAZYSCRIPT_PATH");
  if (spath && spath[0]) {
    char* buf = strdup(spath);
    if (buf) {
      for (char* tok = strtok(buf, ":"); tok != NULL; tok = strtok(NULL, ":")) {
        size_t need = strlen(tok) + 1 + strlen(path) + 1;
        char* cand = (char*)malloc(need);
        if (!cand) break;
        snprintf(cand, need, "%s/%s", tok, path);
        prog = lsparse_file_nullable(cand);
        if (prog) {
          free(buf);
          return prog;
        }
        free(cand);
      }
      free(buf);
    }
  }
  return NULL;
}

lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: require: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return ls_make_err("require: no env");
  lsthunk_t* pathv = ls_eval_arg(args[0], "require: path");
  if (lsthunk_is_err(pathv)) return pathv;
  if (lsthunk_get_type(pathv) != LSTTYPE_STR) {
    return ls_make_err("require: expected string path");
  }
  size_t n=0; char* buf=NULL; FILE* fp=lsopen_memstream_gc(&buf,&n);
  lsstr_print_bare(fp, LSPREC_LOWEST, 0, lsthunk_get_str(pathv)); fclose(fp);
  const char* path = buf;
  if (ls_modules_is_loaded(path)) return ls_make_unit();
  const lsprog_t* prog = ls_require_resolve(path);
  if (!prog) return ls_make_err("require: not found");
  if (ls_effects_get_strict()) ls_effects_begin();
  (void)lsprog_eval(prog, tenv);
  if (ls_effects_get_strict()) ls_effects_end();
  ls_modules_mark_loaded(path);
  return ls_make_unit();
}

// include: evaluate another file and return its resulting value without mutating the
// current environment.  Loading errors are treated as fatal.
lsthunk_t* lsbuiltin_prelude_include(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) { lsprintf(stderr, 0, "F: include: no env\n"); return NULL; }
  lsthunk_t* pathv = ls_eval_arg(args[0], "include: path");
  if (lsthunk_is_err(pathv)) return pathv;
  if (!pathv) { lsprintf(stderr, 0, "F: include: path eval\n"); return NULL; }
  if (lsthunk_get_type(pathv) != LSTTYPE_STR) {
    return ls_make_err("include: expected string path");
  }
  size_t n=0; char* buf=NULL; FILE* fp=lsopen_memstream_gc(&buf,&n);
  lsstr_print_bare(fp, LSPREC_LOWEST, 0, lsthunk_get_str(pathv)); fclose(fp);
  const char* path = buf;
  const lsprog_t* prog = ls_require_resolve(path);
  if (!prog) { lsprintf(stderr, 0, "F: include: not found\n"); return NULL; }
  // Evaluate in an isolated child environment with the current env as parent.
  lstenv_t* child = lstenv_new(tenv);
  // Do NOT enable effects here; pure modules will work, effectful ones will raise at call sites.
  lsthunk_t* ret = lsprog_eval(prog, child);
  if (!ret) { lsprintf(stderr, 0, "F: include: eval\n"); return NULL; }
  return ret;
}

// requirePure removed: use include instead

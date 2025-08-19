#include "runtime/effects.h"
#include "runtime/unit.h"
#include "runtime/modules.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "common/str.h"
#include "common/io.h"
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
  lstenv_t* tenv = (lstenv_t*)data; if (!tenv) return NULL;
  lsthunk_t* pathv = lsthunk_eval0(args[0]); if (!pathv) return NULL;
  if (lsthunk_get_type(pathv) != LSTTYPE_STR) {
    lsprintf(stderr, 0, "E: require: expected string path\n");
    return NULL;
  }
  size_t n=0; char* buf=NULL; FILE* fp=lsopen_memstream_gc(&buf,&n);
  lsstr_print_bare(fp, LSPREC_LOWEST, 0, lsthunk_get_str(pathv)); fclose(fp);
  const char* path = buf;
  if (ls_modules_is_loaded(path)) return ls_make_unit();
  const lsprog_t* prog = ls_require_resolve(path);
  if (!prog) return NULL;
  if (ls_effects_get_strict()) ls_effects_begin();
  (void)lsprog_eval(prog, tenv);
  if (ls_effects_get_strict()) ls_effects_end();
  ls_modules_mark_loaded(path);
  return ls_make_unit();
}

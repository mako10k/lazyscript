// Builtin loader: (~prelude builtin) "name" -> namespace value
#include "builtins/builtin_loader.h"
#include "builtins/ns.h"
#include "runtime/error.h"
#include "runtime/effects.h"
#include "common/io.h"
#include "common/str.h"
#include "common/hash.h"
#include "common/malloc.h"
#include "expr/expr.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

// ABI definition for builtin modules
typedef struct ls_builtin_entry {
  const char* name;   // export name (symbol key)
  int         arity;  // function arity
  lsthunk_t* (*fn)(lssize_t, lsthunk_t* const*, void*);
  void*       data;   // optional user data for the function
} ls_builtin_entry_t;

typedef struct ls_builtin_module {
  int                        abi_version;   // must be 1
  const char*                module_name;   // e.g., "arith"
  const char*                module_version;// optional, e.g., "0.1"
  const ls_builtin_entry_t*  entries;       // array of entries
  int                        entry_count;   // number of entries
  void*                      user_data;     // optional module-level data
} ls_builtin_module_t;

typedef ls_builtin_module_t* (*ls_builtin_init_fn)(void);

// Simple cache: name -> namespace thunk
static lshash_t* g_builtin_cache = NULL;

static int debug_enabled(void) {
  const char* d = getenv("LAZYSCRIPT_DEBUG");
  return d && *d;
}

static int file_exists(const char* path) {
  return access(path, R_OK) == 0;
}

static void get_exe_dir(char* out, size_t outsz) {
  if (!out || outsz == 0) return;
  ssize_t n = readlink("/proc/self/exe", out, outsz - 1);
  if (n <= 0) { out[0] = '\0'; return; }
  out[n] = '\0';
  // strip filename
  for (ssize_t i = n - 1; i >= 0; --i) {
    if (out[i] == '/') { out[i] = '\0'; break; }
  }
}

static void append_path(char* buf, size_t bufsz, const char* dir, const char* base) {
  size_t n = strlen(buf);
  if (n && buf[n-1] != ':') strncat(buf, ":", bufsz - strlen(buf) - 1);
  strncat(buf, dir, bufsz - strlen(buf) - 1);
  if (base && *base) {
    size_t m = strlen(buf);
    if (buf[m-1] != '/') strncat(buf, "/", bufsz - strlen(buf) - 1);
    strncat(buf, base, bufsz - strlen(buf) - 1);
  }
}

static void build_search_paths(char* out, size_t outsz) {
  out[0] = '\0';
  const char* env = getenv("LAZYSCRIPT_BUILTIN_PATH");
  if (env && *env) {
    strncat(out, env, outsz - 1);
  }
  char exedir[PATH_MAX]; exedir[0] = '\0'; get_exe_dir(exedir, sizeof(exedir));
  if (exedir[0]) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s/builtins", exedir);
    append_path(out, outsz, tmp, NULL);
    snprintf(tmp, sizeof(tmp), "%s/plugins", exedir);
    append_path(out, outsz, tmp, NULL);
    // When executed from the build tree, shared objects live in .libs/
    snprintf(tmp, sizeof(tmp), "%s/builtins/.libs", exedir);
    append_path(out, outsz, tmp, NULL);
    snprintf(tmp, sizeof(tmp), "%s/plugins/.libs", exedir);
    append_path(out, outsz, tmp, NULL);
  }
  // Fallback system paths
  append_path(out, outsz, "/usr/local/lib/lazyscript", NULL);
  append_path(out, outsz, "/usr/lib/lazyscript", NULL);
}

static void make_candidate(char* out, size_t outsz, const char* dir, const char* name, int variant) {
  // variant 0: <dir>/liblazyscript_<name>.so
  // variant 1: <dir>/<name>.so
  if (variant == 0)
    snprintf(out, outsz, "%s/liblazyscript_%s.so", dir, name);
  else
    snprintf(out, outsz, "%s/%s.so", dir, name);
}

static lsthunk_t* make_symbol_key(const char* name) {
  // Use .name as zero-arity constructor to be recognized as symbol key
  char buf[256];
  size_t n = strlen(name);
  if (n + 2 >= sizeof(buf)) n = sizeof(buf) - 2; // truncate long names
  buf[0] = '.';
  memcpy(buf + 1, name, n);
  buf[1 + n] = '\0';
  return lsthunk_new_symbol(lsstr_cstr(buf));
}

static lsthunk_t* build_namespace_from_module(const ls_builtin_module_t* mod) {
  if (!mod || !mod->entries || mod->entry_count < 0) return ls_make_err("builtin: invalid module");
  // Build pairs: (key value ...)
  int pairs = mod->entry_count;
  if (pairs <= 0) {
    // empty namespace
    return lsbuiltin_nslit(0, NULL, NULL);
  }
  lsthunk_t** argv = (lsthunk_t**)lsmalloc(sizeof(lsthunk_t*) * (size_t)(pairs * 2));
  for (int i = 0; i < pairs; ++i) {
    const ls_builtin_entry_t* e = &mod->entries[i];
    argv[2*i]   = make_symbol_key(e->name);
    // Value: builtin function thunk
    char label[256];
    const char* mname = mod->module_name ? mod->module_name : "builtin";
    snprintf(label, sizeof(label), "builtin:%s#%s", mname, e->name);
    argv[2*i+1] = lsthunk_new_builtin(lsstr_cstr(label), e->arity, e->fn, e->data);
  }
  lsthunk_t* ns = lsbuiltin_nslit(pairs * 2, argv, NULL);
  // Free arg array; elements managed by thunks
  lsfree(argv);
  return ns;
}

lsthunk_t* lsbuiltin_prelude_builtin(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  if (argc != 1) return ls_make_err("builtin: arity");
  lsthunk_t* namev = ls_eval_arg(args[0], "builtin: name");
  if (lsthunk_is_err(namev)) return namev;
  if (!namev) return ls_make_err("builtin: name eval");
  if (lsthunk_get_type(namev) != LSTTYPE_STR) return ls_make_err("builtin: expected string");
  const lsstr_t* s = lsthunk_get_str(namev);
  const char* modname = lsstr_get_buf(s);
  if (!modname || !*modname) return ls_make_err("builtin: empty name");

  // Cache lookup
  if (!g_builtin_cache) g_builtin_cache = lshash_new(16);
  lshash_data_t cached;
  if (lshash_get(g_builtin_cache, s, &cached)) {
  if (debug_enabled()) lsprintf(stderr, 0, "I: builtin: cache hit: name=%s\n", modname);
    return (lsthunk_t*)cached;
  }

  // If effects are not allowed, fail early (dlopen has side effects)
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: builtin: effect used in pure context (enable seq/chain)\n");
    return NULL; // mimic other effect guards behavior
  }

  // Absolute or explicit .so path
  char chosen[PATH_MAX]; chosen[0] = '\0';
  if (modname[0] == '/' || strstr(modname, "/") || strstr(modname, ".so")) {
    strncpy(chosen, modname, sizeof(chosen) - 1); chosen[sizeof(chosen)-1] = '\0';
  } else {
    // Build search list
    char paths[4096]; build_search_paths(paths, sizeof(paths));
    const char* p = paths;
    while (p && *p) {
      const char* colon = strchr(p, ':');
      size_t len = colon ? (size_t)(colon - p) : strlen(p);
      char dir[PATH_MAX];
      if (len >= sizeof(dir)) len = sizeof(dir) - 1;
      memcpy(dir, p, len); dir[len] = '\0';
      char cand[PATH_MAX];
      make_candidate(cand, sizeof(cand), dir, modname, 0);
      if (file_exists(cand)) { strncpy(chosen, cand, sizeof(chosen)-1); chosen[sizeof(chosen)-1] = '\0'; break; }
      make_candidate(cand, sizeof(cand), dir, modname, 1);
      if (file_exists(cand)) { strncpy(chosen, cand, sizeof(chosen)-1); chosen[sizeof(chosen)-1] = '\0'; break; }
      p = colon ? colon + 1 : NULL;
    }
  }

  if (!chosen[0]) {
    if (debug_enabled()) lsprintf(stderr, 0, "W: builtin: not found: name=%s\n", modname);
    return ls_make_err("builtin: not found");
  }

  void* handle = dlopen(chosen, RTLD_NOW);
  if (!handle) {
    const char* err = dlerror(); (void)err;
    if (debug_enabled()) lsprintf(stderr, 0, "W: builtin: dlopen failed: path=%s err=%s\n", chosen, err ? err : "(null)");
    return ls_make_err("builtin: dlopen failed");
  }
  dlerror();
  ls_builtin_init_fn initfn = (ls_builtin_init_fn)dlsym(handle, "lazyscript_builtin_init");
  const char* derr = dlerror();
  if (derr != NULL || !initfn) {
    // fallback symbol name (optional future): lazyscript_module_init
    dlerror();
    initfn = (ls_builtin_init_fn)dlsym(handle, "lazyscript_module_init");
    if (!initfn) {
      if (debug_enabled()) lsprintf(stderr, 0, "W: builtin: missing init symbol in %s\n", chosen);
      return ls_make_err("builtin: missing init symbol");
    }
  }
  const ls_builtin_module_t* mod = initfn();
  if (!mod) return ls_make_err("builtin: init failed");
  if (mod->abi_version != 1) return ls_make_err("builtin: abi version");

  lsthunk_t* ns = build_namespace_from_module(mod);
  if (lsthunk_is_err(ns) || !ns) return ns ? ns : ls_make_err("builtin: ns build");
  // Cache namespace by module name (copy key so hash owns it)
  const lsstr_t* keycopy = lsstr_new(lsstr_get_buf(s), lsstr_get_len(s));
  lshash_put(g_builtin_cache, keycopy, ns, NULL);
  if (debug_enabled()) lsprintf(stderr, 0, "I: builtin: loaded: name=%s path=%s entries=%d\n", modname, chosen, mod->entry_count);
  return ns;
}

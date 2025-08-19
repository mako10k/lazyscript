#include "lazyscript.h"
#include "common/str.h"
#include "misc/prog.h"
#include "expr/ealge.h"
#include "parser/parser.h"
#include "thunk/thunk.h"
#include <string.h>

#include "parser/lexer.h"
#include <assert.h>
#include <gc.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "coreir/coreir.h"
#include "common/hash.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "common/loc.h"
#include <unistd.h>

static int         g_debug          = 0;
static int         g_effects_strict = 0; // when true, guard side effects
static int         g_effects_depth  = 0; // nesting counter for allowed effects
static int         g_run_main       = 1; // default: on (files). -e path will disable temporarily
static const char* g_entry_name     = "main"; // entry function name
// Global registry for namespaces (avoid peeking into env/thunk internals)
static lshash_t*   g_namespaces     = NULL; // name(lsstr_t*) -> (lsns_t*)

static inline void ls_effects_begin(void) {
  if (g_effects_strict)
    g_effects_depth++;
}
static inline void ls_effects_end(void) {
  if (g_effects_strict && g_effects_depth > 0)
    g_effects_depth--;
}
static inline int  ls_effects_allowed(void) { return !g_effects_strict || g_effects_depth > 0; }
static const char* g_sugar_ns       = NULL; // NULL => default (prelude)
static const char* g_init_file      = NULL; // Optional init script (from --init or env)
static lshash_t*   g_require_loaded = NULL; // cache of loaded module paths
const lsprog_t*    lsparse_stream(const char* filename, FILE* in_str) {
     assert(in_str != NULL);
     yyscan_t yyscanner;
     yylex_init(&yyscanner);
     lsscan_t* lsscan = lsscan_new(filename);
     yyset_in(in_str, yyscanner);
     yyset_extra(lsscan, yyscanner);
     // Apply sugar namespace if configured
     if (g_sugar_ns == NULL)
    g_sugar_ns = getenv("LAZYSCRIPT_SUGAR_NS");
  if (g_sugar_ns && g_sugar_ns[0])
    lsscan_set_sugar_ns(lsscan, g_sugar_ns);
  int             ret  = yyparse(yyscanner);
     const lsprog_t* prog = ret == 0 ? lsscan_get_prog(lsscan) : NULL;
     yylex_destroy(yyscanner);
     return prog;
}

static const lsprog_t* lsparse_file(const char* filename) {
  FILE* stream = fopen(filename, "r");
  if (!stream) {
    perror(filename);
    exit(1);
  }
  const lsprog_t* prog = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

static const lsprog_t* lsparse_string(const char* filename, const char* str) {
  FILE*           stream = fmemopen((void*)str, strlen(str), "r");
  const lsprog_t* prog   = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

// Nullable file parser: returns NULL if file open fails
static const lsprog_t* lsparse_file_nullable(const char* filename) {
  FILE* stream = fopen(filename, "r");
  if (!stream)
    return NULL;
  const lsprog_t* prog = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

// Forward declaration (defined below in Prelude helpers)
static lsthunk_t* ls_make_unit(void);

// Try to run entry function (~g_entry_name) if configured; returns 1 if invoked.
static int ls_maybe_run_entry(lstenv_t* tenv) {
  if (!g_run_main || !tenv || !g_entry_name || !g_entry_name[0])
    return 0;
  lstref_target_t* target = lstenv_get(tenv, lsstr_cstr(g_entry_name));
  if (!target)
    return 0;
  lsloc_t        loc    = lsloc("<entry>", 1, 1, 1, 1);
  const lsref_t* r      = lsref_new(lsstr_cstr(g_entry_name), loc);
  lsthunk_t*     rthunk = lsthunk_new_ref(r, tenv);
  if (g_effects_strict) ls_effects_begin();
  lsthunk_t* v = lsthunk_eval0(rthunk);
  if (v && lsthunk_get_type(v) == LSTTYPE_LAMBDA) {
    lsthunk_t* unit = ls_make_unit();
    (void)lsthunk_eval(v, 1, &unit);
  }
  if (g_effects_strict) ls_effects_end();
  return 1;
}

// Load and evaluate an initialization script into the given environment (thunk path)
static void ls_maybe_eval_init(lstenv_t* tenv) {
  if (g_init_file == NULL || g_init_file[0] == '\0') {
    const char* from_env = getenv("LAZYSCRIPT_INIT");
    if (from_env && from_env[0])
      g_init_file = from_env;
  }
  if (g_init_file && g_init_file[0]) {
    const lsprog_t* iprog = lsparse_file(g_init_file);
    if (iprog) {
      // Effects in init are allowed if strict-effects is enabled (wrap with begin/end)
      if (g_effects_strict)
        ls_effects_begin();
      lsthunk_t* ret = lsprog_eval(iprog, tenv);
      if (g_effects_strict)
        ls_effects_end();
      (void)ret; // ignore value, init is for side effects / definitions
    }
  }
}

static lsthunk_t* lsbuiltin_dump(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_print(stdout, LSPREC_LOWEST, 0, args[0]);
  lsprintf(stdout, 0, "\n");
  return args[0];
}

static lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 1);
  assert(args != NULL);
  size_t len = 0;
  char*  buf = NULL;
  FILE*  fp  = lsopen_memstream_gc(&buf, &len);
  // Evaluate to WHNF so values like ints/strings render as such
  lsthunk_t* v = lsthunk_eval0(args[0]);
  if (v == NULL) {
    fclose(fp);
    return NULL;
  }
  lsthunk_dprint(fp, LSPREC_LOWEST, 0, v);
  fclose(fp);
  const lsstr_t* str = lsstr_new(buf, len);
  return lsthunk_new_str(str);
}

static lsthunk_t* lsbuiltin_print(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 1);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0,
             "E: print: effect used in pure context (enable seq/chain or run with effects)\n");
    return NULL;
  }
  lsthunk_t* thunk_str = lsthunk_eval0(args[0]);
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR)
    thunk_str = lsbuiltin_to_string(1, args, data);
  const lsstr_t* str = lsthunk_get_str(thunk_str);
  lsstr_print_bare(stdout, LSPREC_LOWEST, 0, str);
  return args[0];
}

// --- Prelude helpers/builtins ---
static lsthunk_t* ls_make_unit(void) {
  // Build algebraic constructor () with 0 args
  const lsealge_t* eunit = lsealge_new(lsstr_cstr("()"), 0, NULL);
  return lsthunk_new_ealge(eunit, NULL);
}

// --- Namespaces ------------------------------------------------------------
typedef struct lsns {
  lshash_t* map; // lsstr_t* -> lsthunk_t*
} lsns_t;

// Forward decl used by namespace getter
static lsthunk_t* lsbuiltin_getter0(lssize_t argc, lsthunk_t* const* args, void* data);

static lsthunk_t* lsbuiltin_ns_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  // (~NS sym) => return 0-arity getter for the value bound to sym in NS
  assert(argc == 1);
  assert(args != NULL);
  lsns_t* ns = (lsns_t*)data;
  if (!ns || !ns->map) return NULL;
  lsthunk_t* keyv = lsthunk_eval0(args[0]);
  if (keyv == NULL) return NULL;
  if (lsthunk_get_type(keyv) != LSTTYPE_ALGE || lsthunk_get_argc(keyv) != 0) {
    lsprintf(stderr, 0, "E: namespace: expected bare symbol\n");
    return NULL;
  }
  const lsstr_t* key = lsthunk_get_constr(keyv);
  lshash_data_t  valp;
  if (!lshash_get(ns->map, key, &valp)) {
    lsprintf(stderr, 0, "E: namespace: undefined: ");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, key);
    lsprintf(stderr, 0, "\n");
    return NULL;
  }
  lsthunk_t* val = (lsthunk_t*)valp;
  // Return the value directly
  return val;
}

static lsthunk_t* lsbuiltin_nsnew(lssize_t argc, lsthunk_t* const* args, void* data) {
  // nsnew <BareSymbol>
  assert(argc == 1);
  assert(args != NULL);
  lstenv_t* tenv = (lstenv_t*)data;
  if (tenv == NULL) return NULL;
  lsthunk_t* namev = lsthunk_eval0(args[0]);
  if (namev == NULL) return NULL;
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    lsprintf(stderr, 0, "E: nsnew: expected bare symbol\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  // Create namespace object as a builtin dispatcher capturing its map
  lsns_t* ns = lsmalloc(sizeof(lsns_t));
  ns->map    = lshash_new(16);
  lstenv_put_builtin(tenv, name, 1, lsbuiltin_ns_dispatch, ns);
  if (!g_namespaces)
    g_namespaces = lshash_new(16);
  lshash_data_t oldv;
  (void)lshash_put(g_namespaces, name, (const void*)ns, &oldv);
  return ls_make_unit();
}

static lsthunk_t* lsbuiltin_nsdef(lssize_t argc, lsthunk_t* const* args, void* data) {
  // nsdef <NS-BareSymbol> <BareSymbol> <Value>
  assert(argc == 3);
  assert(args != NULL);
  lstenv_t* tenv = (lstenv_t*)data;
  if (tenv == NULL) return NULL;
  lsthunk_t* nsv = lsthunk_eval0(args[0]);
  lsthunk_t* symv = lsthunk_eval0(args[1]);
  if (nsv == NULL || symv == NULL) return NULL;
  if (lsthunk_get_type(nsv) != LSTTYPE_ALGE || lsthunk_get_argc(nsv) != 0) {
    lsprintf(stderr, 0, "E: nsdef: expected bare symbol for namespace\n");
    return NULL;
  }
  if (lsthunk_get_type(symv) != LSTTYPE_ALGE || lsthunk_get_argc(symv) != 0) {
    lsprintf(stderr, 0, "E: nsdef: expected bare symbol name\n");
    return NULL;
  }
  const lsstr_t* nsname  = lsthunk_get_constr(nsv);
  const lsstr_t* symname = lsthunk_get_constr(symv);
  if (!g_namespaces) {
    lsprintf(stderr, 0, "E: nsdef: namespace not found: ");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, nsname);
    lsprintf(stderr, 0, "\n");
    return NULL;
  }
  lshash_data_t nsp;
  if (!lshash_get(g_namespaces, nsname, &nsp)) {
    lsprintf(stderr, 0, "E: nsdef: namespace not found: ");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, nsname);
    lsprintf(stderr, 0, "\n");
    return NULL;
  }
  lsns_t* ns = (lsns_t*)nsp;
  if (!ns || !ns->map) return NULL;
  // Stabilize value and store
  lsthunk_t* val = lsthunk_eval0(args[2]);
  if (val == NULL) return NULL;
  lshash_data_t oldv;
  (void)lshash_put(ns->map, symname, (const void*)val, &oldv);
  return ls_make_unit();
}

static lsthunk_t* lsbuiltin_prelude_exit(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: exit: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* val = lsthunk_eval0(args[0]);
  if (val == NULL)
    return NULL;
  if (lsthunk_get_type(val) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: exit: invalid type\n");
    return NULL;
  }
  // We only distinguish zero/non-zero to avoid needing a raw int getter.
  int is_zero = lsint_eq(lsthunk_get_int(val), lsint_new(0));
  exit(is_zero ? 0 : 1);
}

static lsthunk_t* lsbuiltin_prelude_println(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: println: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lsthunk_t* thunk_str = lsthunk_eval0(args[0]);
  if (thunk_str == NULL)
    return NULL;
  if (lsthunk_get_type(thunk_str) != LSTTYPE_STR)
    thunk_str = lsbuiltin_to_string(1, args, NULL);
  const lsstr_t* str = lsthunk_get_str(thunk_str);
  lsstr_print_bare(stdout, LSPREC_LOWEST, 0, str);
  lsprintf(stdout, 0, "\n");
  return ls_make_unit();
}

static lsthunk_t* lsbuiltin_prelude_chain(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  // Evaluate the first action for effects
  ls_effects_begin();
  lsthunk_t* action = lsthunk_eval0(args[0]);
  ls_effects_end();
  if (action == NULL)
    return NULL;
  // Apply the continuation to unit
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

static lsthunk_t* lsbuiltin_prelude_return(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 1);
  assert(args != NULL);
  return args[0];
}

// prelude.require: load and evaluate a LazyScript file at runtime (side-effect)
static lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 1);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: require: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lstenv_t* tenv = (lstenv_t*)data;
  if (tenv == NULL)
    return NULL;
  lsthunk_t* pathv = lsthunk_eval0(args[0]);
  if (pathv == NULL)
    return NULL;
  if (lsthunk_get_type(pathv) != LSTTYPE_STR) {
    lsprintf(stderr, 0, "E: require: expected string path\n");
    return NULL;
  }
  const lsstr_t* s   = lsthunk_get_str(pathv);
  size_t         n   = 0;
  char*          buf = NULL;
  FILE*          fp  = lsopen_memstream_gc(&buf, &n);
  lsstr_print_bare(fp, LSPREC_LOWEST, 0, s);
  fclose(fp);
  const char* path = buf;
  // Resolve path: absolute/relative or via LAZYSCRIPT_PATH
  const char*     resolved = NULL;
  const lsprog_t* prog     = NULL;
  // try as-is
  prog = lsparse_file_nullable(path);
  if (prog) {
    resolved = path;
  } else {
    const char* spath = getenv("LAZYSCRIPT_PATH");
    if (spath && spath[0]) {
      // copy spath to buffer to tokenize
      char* buf = strdup(spath);
      if (buf) {
        for (char* tok = strtok(buf, ":"); tok != NULL; tok = strtok(NULL, ":")) {
          size_t need = strlen(tok) + 1 + strlen(path) + 1;
          char*  cand = (char*)malloc(need);
          if (!cand)
            break;
          snprintf(cand, need, "%s/%s", tok, path);
          prog = lsparse_file_nullable(cand);
          if (prog) {
            resolved = cand;
            break;
          }
          free(cand);
        }
        free(buf);
      }
    }
  }
  if (!prog)
    return NULL;
  // module loaded cache
  if (!g_require_loaded)
    g_require_loaded = lshash_new(16);
  const lsstr_t* k = lsstr_cstr(resolved);
  lshash_data_t  oldv;
  if (lshash_get(g_require_loaded, k, &oldv)) {
    // already loaded; do nothing
    return ls_make_unit();
  }
  (void)lshash_put(g_require_loaded, k, (const void*)1, &oldv);
  if (g_effects_strict)
    ls_effects_begin();
  lsthunk_t* ret = lsprog_eval(prog, tenv);
  if (g_effects_strict)
    ls_effects_end();
  (void)ret;
  return ls_make_unit();
}

// 0-arity builtin getter for prelude.def: returns captured thunk value
static lsthunk_t* lsbuiltin_getter0(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)args;
  return (lsthunk_t*)data;
}

// prelude.def: bind a value to a bare symbol in current environment
static lsthunk_t* lsbuiltin_prelude_def(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 2);
  assert(args != NULL);
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: def: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  lstenv_t* tenv = (lstenv_t*)data;
  if (tenv == NULL)
    return NULL;
  lsthunk_t* namev = lsthunk_eval0(args[0]);
  if (namev == NULL)
    return NULL;
  if (lsthunk_get_type(namev) != LSTTYPE_ALGE || lsthunk_get_argc(namev) != 0) {
    lsprintf(stderr, 0, "E: def: expected a bare symbol as first argument\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(namev);
  // value: allow lazy or strict? evaluate now to stabilize
  lsthunk_t* val = lsthunk_eval0(args[1]);
  if (val == NULL)
    return NULL;
  // Install as 0-arity builtin that returns captured value
  lstenv_put_builtin(tenv, name, 0, lsbuiltin_getter0, val);
  return ls_make_unit();
}

static lsthunk_t* lsbuiltin_prelude_bind(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data;
  assert(argc == 2);
  assert(args != NULL);
  // Evaluate the first computation to get its value (with effects enabled)
  ls_effects_begin();
  lsthunk_t* val = lsthunk_eval0(args[0]);
  ls_effects_end();
  if (val == NULL)
    return NULL;
  // Apply the continuation to the value
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &val);
}

static lsthunk_t* lsbuiltin_prelude_dispatch(lssize_t argc, lsthunk_t* const* args, void* data) {
  // data is the current environment for dynamic loading (require)
  lstenv_t* tenv = (lstenv_t*)data;
  assert(argc == 1);
  assert(args != NULL);
  lsthunk_t* key = lsthunk_eval0(args[0]);
  if (key == NULL)
    return NULL;
  if (lsthunk_get_type(key) != LSTTYPE_ALGE || lsthunk_get_argc(key) != 0) {
    lsprintf(stderr, 0, "E: prelude: expected a bare symbol (e.g., exit/println/chain/return)\n");
    return NULL;
  }
  const lsstr_t* name = lsthunk_get_constr(key);
  if (lsstrcmp(name, lsstr_cstr("exit")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.exit"), 1, lsbuiltin_prelude_exit, NULL);
  if (lsstrcmp(name, lsstr_cstr("println")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.println"), 1, lsbuiltin_prelude_println, NULL);
  if (lsstrcmp(name, lsstr_cstr("def")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.def"), 2, lsbuiltin_prelude_def, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsnew")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsnew"), 1, lsbuiltin_nsnew, tenv);
  if (lsstrcmp(name, lsstr_cstr("nsdef")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.nsdef"), 3, lsbuiltin_nsdef, tenv);
  if (lsstrcmp(name, lsstr_cstr("require")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.require"), 1, lsbuiltin_prelude_require, tenv);
  if (lsstrcmp(name, lsstr_cstr("chain")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.chain"), 2, lsbuiltin_prelude_chain, NULL);
  if (lsstrcmp(name, lsstr_cstr("bind")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.bind"), 2, lsbuiltin_prelude_bind, NULL);
  if (lsstrcmp(name, lsstr_cstr("return")) == 0)
    return lsthunk_new_builtin(lsstr_cstr("prelude.return"), 1, lsbuiltin_prelude_return, NULL);
  lsprintf(stderr, 0, "E: prelude: unknown symbol: ");
  lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
  lsprintf(stderr, 0, "\n");
  return NULL;
}

typedef enum lsseq_type {
  LSSEQ_SIMPLE,
  LSSEQ_CHAIN,
} lsseq_type_t;

static lsthunk_t* lsbuiltin_seq(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* fst = args[0];
  lsthunk_t* snd = args[1];
  ls_effects_begin();
  lsthunk_t* fst_evaled = lsthunk_eval0(fst);
  ls_effects_end();
  if (fst_evaled == NULL)
    return NULL;
  switch ((lsseq_type_t)(intptr_t)data) {
  case LSSEQ_SIMPLE: // Simple seq
    return snd;
  case LSSEQ_CHAIN: { // Chain seq
    lsthunk_t* fst_new =
        lsthunk_new_builtin(lsstr_cstr("seqc"), 2, lsbuiltin_seq, (void*)LSSEQ_CHAIN);
    lsthunk_t* ret = lsthunk_eval(fst_new, 1, &snd);
    return ret;
  }
  }
}

static lsthunk_t* lsbuiltin_add(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* lhs = lsthunk_eval0(args[0]);
  lsthunk_t* rhs = lsthunk_eval0(args[1]);
  if (lhs == NULL || rhs == NULL)
    return NULL;
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: add: invalid type\n");
    return NULL;
  }
  return lsthunk_new_int(lsint_add(lsthunk_get_int(lhs), lsthunk_get_int(rhs)));
}

static lsthunk_t* lsbuiltin_sub(lssize_t argc, lsthunk_t* const* args, void* data) {
  assert(argc == 2);
  assert(args != NULL);
  lsthunk_t* lhs = lsthunk_eval0(args[0]);
  lsthunk_t* rhs = lsthunk_eval0(args[1]);
  if (lhs == NULL || rhs == NULL)
    return NULL;
  if (lsthunk_get_type(lhs) != LSTTYPE_INT || lsthunk_get_type(rhs) != LSTTYPE_INT) {
    lsprintf(stderr, 0, "E: sub: invalid type\n");
    return NULL;
  }
  return lsthunk_new_int(lsint_sub(lsthunk_get_int(lhs), lsthunk_get_int(rhs)));
}

static void ls_register_core_builtins(lstenv_t* tenv) {
  lstenv_put_builtin(tenv, lsstr_cstr("dump"), 1, lsbuiltin_dump, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("to_str"), 1, lsbuiltin_to_string, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("print"), 1, lsbuiltin_print, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("seq"), 2, lsbuiltin_seq, (void*)LSSEQ_SIMPLE);
  lstenv_put_builtin(tenv, lsstr_cstr("seqc"), 2, lsbuiltin_seq, (void*)LSSEQ_CHAIN);
  lstenv_put_builtin(tenv, lsstr_cstr("add"), 2, lsbuiltin_add, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("sub"), 2, lsbuiltin_sub, NULL);
  // namespaces
  lstenv_put_builtin(tenv, lsstr_cstr("nsnew"), 1, lsbuiltin_nsnew, tenv);
  lstenv_put_builtin(tenv, lsstr_cstr("nsdef"), 3, lsbuiltin_nsdef, tenv);
}

static void ls_register_builtin_prelude(lstenv_t* tenv) {
  // Pass tenv via data to allow prelude.require to load into this environment
  lstenv_put_builtin(tenv, lsstr_cstr("prelude"), 1, lsbuiltin_prelude_dispatch, tenv);
}

typedef int (*ls_prelude_register_fn)(lstenv_t*);

static int ls_try_load_prelude_plugin(lstenv_t* tenv, const char* path) {
  const char* chosen = path;
  if (chosen == NULL)
    chosen = getenv("LAZYSCRIPT_PRELUDE_SO");
  if (chosen == NULL || chosen[0] == '\0')
    return 0;
  void* handle = dlopen(chosen, RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    lsprintf(stderr, 0, "W: dlopen failed: %s\n", dlerror());
    return 0;
  }
  dlerror();
  ls_prelude_register_fn reg = (ls_prelude_register_fn)dlsym(handle, "ls_prelude_register");
  const char*            err = dlerror();
  if (err != NULL || reg == NULL) {
    lsprintf(stderr, 0, "W: dlsym(ls_prelude_register) failed: %s\n", err ? err : "(null)");
    dlclose(handle);
    return 0;
  }
  int rc = reg(tenv);
  if (rc != 0) {
    lsprintf(stderr, 0, "W: prelude plugin returned error: %d\n", rc);
    // keep handle but report error; fall back
    return 0;
  }
  // Keep handle open for the lifetime of process
  return 1;
}

int main(int argc, char** argv) {
  const char*   prelude_so       = NULL;
  int           dump_coreir      = 0;
  int           eval_coreir      = 0;
  int           typecheck_coreir = 0;
  int           kind_warn        = 1; // default warn
  int           kind_error       = 0; // default no error
  struct option longopts[]       = {
          { "eval", required_argument, NULL, 'e' },
          { "prelude-so", required_argument, NULL, 'p' },
          { "sugar-namespace", required_argument, NULL, 'n' },
          { "strict-effects", no_argument, NULL, 's' },
          { "run-main", no_argument, NULL, 1003 },
          { "entry", required_argument, NULL, 1004 },
          { "dump-coreir", no_argument, NULL, 'i' },
          { "eval-coreir", no_argument, NULL, 'c' },
          { "typecheck", no_argument, NULL, 't' },
          { "no-kind-warn", no_argument, NULL, 1000 },
          { "kind-error", no_argument, NULL, 1001 },
          { "init", required_argument, NULL, 1002 },
          { "debug", no_argument, NULL, 'd' },
          { "help", no_argument, NULL, 'h' },
          { "version", no_argument, NULL, 'v' },
          { NULL, 0, NULL, 0 },
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "e:p:n:sictdhv", longopts, NULL)) != -1) {
    switch (opt) {
    case 'e': {
      static int eval_count = 0;
      char       name[32];
      snprintf(name, sizeof(name), "<eval:#%d>", ++eval_count);
      const lsprog_t* prog = lsparse_string(name, optarg);
      if (prog != NULL) {
        if (dump_coreir) {
          const lscir_prog_t* cir = lscir_lower_prog(prog);
          lscir_print(stdout, 0, cir);
          if (g_effects_strict) {
            int errs = lscir_validate_effects(stderr, cir);
            if (errs > 0) {
              fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
              exit(1);
            }
          }
          break;
        }
        if (g_debug) {
          lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
        }
        if (typecheck_coreir) {
          lscir_typecheck_set_kind_warn(kind_warn);
          lscir_typecheck_set_kind_error(kind_error);
          const lscir_prog_t* cir = lscir_lower_prog(prog);
          if (g_effects_strict) {
            int errs = lscir_validate_effects(stderr, cir);
            if (errs > 0) {
              fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
              exit(1);
            }
          }
          return lscir_typecheck(stdout, cir);
        }
        if (eval_coreir) {
          const lscir_prog_t* cir = lscir_lower_prog(prog);
          if (g_effects_strict) {
            int errs = lscir_validate_effects(stderr, cir);
            if (errs > 0) {
              fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
              exit(1);
            }
          }
          lscir_eval(stdout, cir);
          break;
        }
        if (g_effects_strict) {
          const lscir_prog_t* cir  = lscir_lower_prog(prog);
          int                 errs = lscir_validate_effects(stderr, cir);
          if (errs > 0) {
            fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
            exit(1);
          }
        }
        lstenv_t* tenv = lstenv_new(NULL);
        ls_register_core_builtins(tenv);
        if (!ls_try_load_prelude_plugin(tenv, prelude_so))
          ls_register_builtin_prelude(tenv);
        int saved_run_main = g_run_main;
        g_run_main         = 0; // -e は従来通り：最終値を出力
        lsthunk_t* ret     = lsprog_eval(prog, tenv);
        if (ret != NULL) {
          lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
          lsprintf(stdout, 0, "\n");
        }
        g_run_main = saved_run_main;
      }
      break;
    }
    case 'p':
      prelude_so = optarg;
      break;
    case 'n':
      g_sugar_ns = optarg;
      break;
    case 's':
      g_effects_strict = 1;
      break;
    case 1003: // --run-main
      g_run_main = 1;
      break;
    case 1004: // --entry <name>
      g_entry_name = optarg;
      break;
    case 'i':
      dump_coreir = 1;
      break;
    case 'c':
      eval_coreir = 1;
      break;
    case 't':
      typecheck_coreir = 1;
      break;
    case 1000: // --no-kind-warn
      kind_warn = 0;
      break;
    case 1001: // --kind-error
      kind_error = 1;
      break;
    case 1002: // --init <file>
      g_init_file = optarg;
      break;
    case 'd':
      g_debug = 1;
#if DEBUG
      {
        extern int yydebug;
        yydebug = 1;
      }
#endif
      break;
    case 'h':
      printf("Usage: %s [OPTION]... [FILE]...\n", argv[0]);
      printf("Options:\n");
      printf("  -d, --debug     print debug information\n");
      printf("  -e, --eval      evaluate a one-line program string\n");
      printf("  -p, --prelude-so <path>  load prelude plugin .so (override)\n");
      printf("  -n, --sugar-namespace <ns>  set namespace for ~~sym sugar (default: prelude)\n");
      printf("  -s, --strict-effects  enforce effect discipline (seq/chain required)\n");
  printf("      --run-main          run entry function instead of printing top-level value (off)\n");
  printf("      --entry <name>      set entry function name (default: main)\n");
      printf("  -i, --dump-coreir  print Core IR after parsing (debug)\n");
      printf("  -c, --eval-coreir  run via Core IR evaluator (smoke)\n");
      printf("  -t, --typecheck    run minimal Core IR typechecker and print OK/error\n");
      printf("      --no-kind-warn  suppress kind (Pure/IO) warnings during --typecheck\n");
      printf("      --kind-error    treat kind (Pure/IO) issues as errors during --typecheck\n");
      printf("      --init <file>   load and evaluate an init LazyScript before user code (thunk "
             "path)\n");
      printf("  -h, --help      display this help and exit\n");
      printf("  -v, --version   output version information and exit\n");
      printf("\nEnvironment:\n  LAZYSCRIPT_PRELUDE_SO  path to prelude plugin .so (used if -p not "
             "set)\n");
      printf("  LAZYSCRIPT_SUGAR_NS     namespace used for ~~sym sugar (if -n not set)\n");
      printf("  LAZYSCRIPT_INIT         path to init LazyScript (used if --init not set)\n");
      exit(0);
    case 'v':
      printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      exit(0);
    default:
      exit(1);
    }
  }
  for (int i = optind; i < argc; i++) {
    const char* filename = argv[i];
    if (strcmp(filename, "-") == 0)
      filename = "/dev/stdin";
    const lsprog_t* prog = lsparse_file(filename);
    if (prog != NULL) {
      if (dump_coreir) {
        const lscir_prog_t* cir = lscir_lower_prog(prog);
        lscir_print(stdout, 0, cir);
        if (g_effects_strict) {
          int errs = lscir_validate_effects(stderr, cir);
          if (errs > 0) {
            fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
            exit(1);
          }
        }
        continue;
      }
      if (g_debug) {
        lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
      }
      if (typecheck_coreir) {
        lscir_typecheck_set_kind_warn(kind_warn);
        lscir_typecheck_set_kind_error(kind_error);
        const lscir_prog_t* cir = lscir_lower_prog(prog);
        if (g_effects_strict) {
          int errs = lscir_validate_effects(stderr, cir);
          if (errs > 0) {
            fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
            exit(1);
          }
        }
        int rc = lscir_typecheck(stdout, cir);
        if (rc != 0)
          return rc;
        continue;
      }
      if (eval_coreir) {
        const lscir_prog_t* cir = lscir_lower_prog(prog);
        if (g_effects_strict) {
          int errs = lscir_validate_effects(stderr, cir);
          if (errs > 0) {
            fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
            exit(1);
          }
        }
        lscir_eval(stdout, cir);
        continue;
      }
      if (g_effects_strict) {
        const lscir_prog_t* cir  = lscir_lower_prog(prog);
        int                 errs = lscir_validate_effects(stderr, cir);
        if (errs > 0) {
          fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
          exit(1);
        }
      }
      lstenv_t* tenv = lstenv_new(NULL);
      ls_register_core_builtins(tenv);
      if (!ls_try_load_prelude_plugin(tenv, prelude_so))
        ls_register_builtin_prelude(tenv);
      // Evaluate init script (if any) into the same environment
      ls_maybe_eval_init(tenv);
      // Evaluate init script (if any) into the same environment
      ls_maybe_eval_init(tenv);
      lsthunk_t* ret = lsprog_eval(prog, tenv);
      if (ret != NULL && !ls_maybe_run_entry(tenv)) {
        lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
        lsprintf(stdout, 0, "\n");
      }
    }
  }
  return 0;
}
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
#include "runtime/effects.h"
#include "runtime/unit.h"
// builtins modules
#include "builtins/prelude.h"
#include "builtins/ns.h"

static int         g_debug          = 0;
static int         g_run_main       = 1; // default: on (files). -e path will disable temporarily
static const char* g_entry_name     = "main"; // entry function name
// Global registry for namespaces (avoid peeking into env/thunk internals)
static lshash_t*   g_namespaces     = NULL; // name(lsstr_t*) -> (lsns_t*)

// effects helpers moved to runtime/effects.{h,c}
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
// moved to runtime/unit.h as inline

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
  if (ls_effects_get_strict()) ls_effects_begin();
  lsthunk_t* v = lsthunk_eval0(rthunk);
  if (v && lsthunk_get_type(v) == LSTTYPE_LAMBDA) {
    lsthunk_t* unit = ls_make_unit();
    (void)lsthunk_eval(v, 1, &unit);
  }
  if (ls_effects_get_strict()) ls_effects_end();
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
  if (ls_effects_get_strict())
        ls_effects_begin();
      lsthunk_t* ret = lsprog_eval(iprog, tenv);
  if (ls_effects_get_strict())
        ls_effects_end();
      (void)ret; // ignore value, init is for side effects / definitions
    }
  }
}

// builtin symbols defined in separate modules
lsthunk_t* lsbuiltin_dump(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_print(lssize_t argc, lsthunk_t* const* args, void* data);

// --- Prelude helpers/builtins ---

// Namespaces builtins are now provided by builtins/ns.c

// prelude.require: load and evaluate a LazyScript file at runtime (side-effect)
lsthunk_t* lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data) {
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
  if (ls_effects_get_strict())
    ls_effects_begin();
  lsthunk_t* ret = lsprog_eval(prog, tenv);
  if (ls_effects_get_strict())
    ls_effects_end();
  (void)ret;
  return ls_make_unit();
}
// Prelude builtins are implemented in builtins/prelude.c

// seq is implemented in builtins/seq.c

// arithmetic builtins are in builtins/arith.c

static void ls_register_core_builtins(lstenv_t* tenv) {
  lstenv_put_builtin(tenv, lsstr_cstr("dump"), 1, lsbuiltin_dump, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("to_str"), 1, lsbuiltin_to_string, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("print"), 1, lsbuiltin_print, NULL);
  extern lsthunk_t* lsbuiltin_seq(lssize_t,lsthunk_t* const*,void*);
  lstenv_put_builtin(tenv, lsstr_cstr("seq"), 2, lsbuiltin_seq, (void*)0);
  lstenv_put_builtin(tenv, lsstr_cstr("seqc"), 2, lsbuiltin_seq, (void*)1);
  extern lsthunk_t* lsbuiltin_add(lssize_t,lsthunk_t* const*,void*);
  extern lsthunk_t* lsbuiltin_sub(lssize_t,lsthunk_t* const*,void*);
  lstenv_put_builtin(tenv, lsstr_cstr("add"), 2, lsbuiltin_add, NULL);
  lstenv_put_builtin(tenv, lsstr_cstr("sub"), 2, lsbuiltin_sub, NULL);
  // namespaces
  lstenv_put_builtin(tenv, lsstr_cstr("nsnew"), 1, lsbuiltin_nsnew, tenv);
  lstenv_put_builtin(tenv, lsstr_cstr("nsdef"), 3, lsbuiltin_nsdef, tenv);
}

// moved to builtins/prelude.c: ls_register_builtin_prelude

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
          if (ls_effects_get_strict()) {
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
          if (ls_effects_get_strict()) {
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
          if (ls_effects_get_strict()) {
            int errs = lscir_validate_effects(stderr, cir);
            if (errs > 0) {
              fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
              exit(1);
            }
          }
          lscir_eval(stdout, cir);
          break;
        }
  if (ls_effects_get_strict()) {
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
  ls_effects_set_strict(1);
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
  if (ls_effects_get_strict()) {
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
  if (ls_effects_get_strict()) {
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
  if (ls_effects_get_strict()) {
          int errs = lscir_validate_effects(stderr, cir);
          if (errs > 0) {
            fprintf(stderr, "E: strict-effects: %d error(s)\n", errs);
            exit(1);
          }
        }
        lscir_eval(stdout, cir);
        continue;
      }
  if (ls_effects_get_strict()) {
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
      lsthunk_t* ret = lsprog_eval(prog, tenv);
      if (ret != NULL && !ls_maybe_run_entry(tenv)) {
        lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
        lsprintf(stdout, 0, "\n");
      }
    }
  }
  return 0;
}
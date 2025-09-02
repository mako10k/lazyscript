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
/* coreir removed */
#include "common/hash.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "common/loc.h"
#include <unistd.h>
#include <limits.h>
#include "runtime/effects.h"
#include "runtime/unit.h"
#include "runtime/error.h"
// builtins modules
#include "builtins/ns.h"
#include "runtime/builtin.h"
#include "runtime/trace.h"

static int         g_debug             = 0;
static int         g_run_main          = 1; // default: on (files). -e path will disable temporarily
static const char* g_entry_name        = "main"; // entry function name
static int         g_trace_stack_depth = 1;      // default: print top frame only
static const char* g_trace_dump_path =
    NULL; // optional: where to emit JSONL in thunk creation order
// Global registry for namespaces moved to builtins/ns.c

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

const lsprog_t* lsparse_file(const char* filename) {
  FILE* stream = fopen(filename, "r");
  if (!stream) {
    perror(filename);
    exit(1);
  }
  const lsprog_t* prog = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

const lsprog_t* lsparse_string(const char* filename, const char* str) {
  FILE*           stream = fmemopen((void*)str, strlen(str), "r");
  const lsprog_t* prog   = lsparse_stream(filename, stream);
  fclose(stream);
  return prog;
}

// Nullable file parser: returns NULL if file open fails
const lsprog_t* lsparse_file_nullable(const char* filename) {
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
  if (ls_effects_get_strict())
    ls_effects_begin();
  lsthunk_t* v = lsthunk_eval0(rthunk);
  if (v && lsthunk_get_type(v) == LSTTYPE_LAMBDA) {
    lsthunk_t* unit = ls_make_unit();
    (void)lsthunk_eval(v, 1, &unit);
  }
  if (ls_effects_get_strict())
    ls_effects_end();
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

// builtin prototypes are provided via runtime/builtin.h

// --- Prelude helpers/builtins ---

// Namespaces builtins are now provided by builtins/ns.c

// prelude.require は builtins/require.c に移動
// Prelude builtins are provided via the prelude plugin only
// include の実装も require.c にあるため、ここで使用宣言
lsthunk_t* lsbuiltin_prelude_include(lssize_t argc, lsthunk_t* const* args, void* data);

// seq is implemented in builtins/seq.c

// arithmetic builtins are in builtins/arith.c
lsthunk_t* lsbuiltin_lt(lssize_t argc, lsthunk_t* const* args, void* data);

// Prelude is provided via plugin: see src/plugins/prelude_plugin.c

typedef int (*ls_prelude_register_fn)(lstenv_t*);

static void get_exe_dir(char* out, size_t outsz) {
  if (!out || outsz == 0)
    return;
  ssize_t n = readlink("/proc/self/exe", out, outsz - 1);
  if (n <= 0) {
    out[0] = '\0';
    return;
  }
  out[n] = '\0';
  for (ssize_t i = n - 1; i >= 0; --i) {
    if (out[i] == '/') {
      out[i] = '\0';
      break;
    }
  }
}

static int file_exists(const char* path) { return access(path, R_OK) == 0; }

// safe join helper to avoid -Wformat-truncation on snprintf
static int join2(char* out, size_t outsz, const char* a, const char* b) {
  if (!out || outsz == 0)
    return 0;
  size_t al = a ? strnlen(a, outsz) : 0;
  size_t bl = b ? strlen(b) : 0;
  if (al + bl >= outsz) {
    out[0] = '\0';
    return 0;
  }
  if (a && al)
    memcpy(out, a, al);
  if (b && bl)
    memcpy(out + al, b, bl);
  out[al + bl] = '\0';
  return 1;
}

static const char* ls_find_prelude_so(char* buf, size_t bufsz) {
  if (!buf || bufsz == 0)
    return NULL;
  buf[0] = '\0';

  const char* envp = getenv("LAZYSCRIPT_PRELUDE_PATH");
  if (envp && envp[0]) {
    const char* p = envp;
    while (p && *p) {
      const char* colon = strchr(p, ':');
      size_t      len   = colon ? (size_t)(colon - p) : strlen(p);
      char        dir[PATH_MAX];
      if (len >= sizeof(dir))
        len = sizeof(dir) - 1;
      memcpy(dir, p, len);
      dir[len] = '\0';
      if (!join2(buf, bufsz, dir, "/liblazyscript_prelude.so")) {
        buf[0] = '\0';
      }
      if (file_exists(buf))
        return buf;
      p = colon ? colon + 1 : NULL;
    }
  }
  char exedir[PATH_MAX];
  exedir[0] = '\0';
  get_exe_dir(exedir, sizeof(exedir));
  if (exedir[0]) {
    if (!join2(buf, bufsz, exedir, "/plugins/liblazyscript_prelude.so")) {
      buf[0] = '\0';
    }
    if (file_exists(buf))
      return buf;
    // When running from build tree, plugin resides in .libs/
    if (!join2(buf, bufsz, exedir, "/plugins/.libs/liblazyscript_prelude.so")) {
      buf[0] = '\0';
    }
    if (file_exists(buf))
      return buf;
    // Also try parent dir (exedir is usually src/.libs, plugin is in src/plugins/.libs)
    char parent[PATH_MAX];
    memcpy(parent, exedir, sizeof(parent));
    parent[sizeof(parent) - 1] = '\0';
    for (ssize_t i = (ssize_t)strlen(parent) - 1; i >= 0; --i) {
      if (parent[i] == '/') {
        parent[i] = '\0';
        break;
      }
    }
    if (!join2(buf, bufsz, parent, "/plugins/.libs/liblazyscript_prelude.so")) {
      buf[0] = '\0';
    }
    if (file_exists(buf))
      return buf;
  }
  snprintf(buf, bufsz, "/usr/local/lib/lazyscript/liblazyscript_prelude.so");
  if (file_exists(buf))
    return buf;
  snprintf(buf, bufsz, "/usr/lib/lazyscript/liblazyscript_prelude.so");
  if (file_exists(buf))
    return buf;
  buf[0] = '\0';
  return NULL;
}

static int ls_try_load_prelude_plugin(lstenv_t* tenv, const char* path) {
  const char* chosen = path;
  const char* source = NULL; // for logging
  if (chosen == NULL)
    chosen = getenv("LAZYSCRIPT_PRELUDE_SO");
  if (chosen && chosen[0])
    source = "CLI/ENV";
  char found[PATH_MAX];
  if ((chosen == NULL || chosen[0] == '\0')) {
    // Try to find via LAZYSCRIPT_PRELUDE_PATH / standard locations
    if (ls_find_prelude_so(found, sizeof(found))) {
      chosen = found;
      source = "AUTO";
    }
  }
  if (chosen == NULL || chosen[0] == '\0') {
    if (g_debug)
      lsprintf(stderr, 0, "I: prelude: plugin not found (auto-discovery)\n");
    return 0;
  }
  void* handle = dlopen(chosen, RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    const char* err = dlerror();
    lsprintf(stderr, 0, "W: prelude: dlopen failed: path=%s err=%s\n", chosen,
             err ? err : "(null)");
    return 0;
  }
  dlerror();
  ls_prelude_register_fn reg = (ls_prelude_register_fn)dlsym(handle, "ls_prelude_register");
  const char*            err = dlerror();
  if (err != NULL || reg == NULL) {
    lsprintf(stderr, 0, "W: prelude: dlsym(ls_prelude_register) failed: %s\n",
             err ? err : "(null)");
    dlclose(handle);
    return 0;
  }
  int rc = reg(tenv);
  if (rc != 0) {
    lsprintf(stderr, 0, "W: prelude: plugin returned error: %d\n", rc);
    // keep handle but report error; fall back
    return 0;
  }
  if (g_debug) {
    lsprintf(stderr, 0, "I: prelude: plugin loaded: path=%s source=%s\n", chosen,
             source ? source : "(n/a)");
  }
  // Keep handle open for the lifetime of process
  return 1;
}

// --- Prelude as value setup ---
// local 0-arity getter: returns captured thunk value
static lsthunk_t* lsbuiltin_getter0_local(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  (void)args;
  return (lsthunk_t*)data;
}

// local applier: delegates application to captured thunk (namespace/thunk value)
static lsthunk_t* lsbuiltin_apply_thunk(lssize_t argc, lsthunk_t* const* args, void* data) {
  lsthunk_t* val = (lsthunk_t*)data;
  if (!val)
    return NULL;
  if (argc == 0)
    return lsthunk_eval0(val);
  return lsthunk_eval(val, argc, args);
}

// nslit builtin is provided by ns.c and dispatched by libraries/plugins as needed
extern lsthunk_t* lsbuiltin_nslit(lssize_t argc, lsthunk_t* const* args, void* data);

// Forward decls from require/builtin loader
lsthunk_t* lsbuiltin_prelude_builtin(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_prelude_include(lssize_t argc, lsthunk_t* const* args, void* data);

int        main(int argc, char** argv) {
         // Ensure Boehm GC is initialized early, before any allocations in flex/bison
  // scanner/parser (yylex_init may allocate and trigger GC lazy init otherwise).
  // If libc allocator is requested, skip GC_init to avoid unnecessary GC setup
  // and potential environment-specific issues.
  const char* _ls_use_libc = getenv("LAZYSCRIPT_USE_LIBC_ALLOC");
  if (!(_ls_use_libc && _ls_use_libc[0] && _ls_use_libc[0] != '0')) {
           GC_init();
  }
  const char*   prelude_so     = NULL;
  int           kind_warn      = 1;    // (kept for future, no coreir)
  int           kind_error     = 0;    // (kept for future, no coreir)
  const char*   trace_map_path = NULL; // optional sourcemap for runtime tracing
  struct option longopts[]     = {
               { "eval", required_argument, NULL, 'e' },
               { "prelude-so", required_argument, NULL, 'p' },
               { "sugar-namespace", required_argument, NULL, 'n' },
               { "strict-effects", no_argument, NULL, 's' },
               { "run-main", no_argument, NULL, 1003 },
               { "entry", required_argument, NULL, 1004 },
               { "init", required_argument, NULL, 1002 },
               { "trace-map", required_argument, NULL, 2000 },
               { "trace-stack-depth", required_argument, NULL, 2001 },
               { "trace-dump", required_argument, NULL, 2002 },
               { "debug", no_argument, NULL, 'd' },
               { "help", no_argument, NULL, 'h' },
               { "version", no_argument, NULL, 'v' },
               { NULL, 0, NULL, 0 },
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "e:p:n:sdhv", longopts, NULL)) != -1) {
           switch (opt) {
           case 'e': {
             // Evaluate one-line program
      const lsprog_t* prog = lsparse_string("<cmd>", optarg);
      if (prog != NULL) {
               if (g_debug)
          lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
        lstenv_t* tenv = lstenv_new(NULL);
               if (!ls_try_load_prelude_plugin(tenv, prelude_so)) {
                 lsprintf(stderr, 0,
                          "E: prelude: plugin not found or failed to load; set --prelude-so or install "
                                 "liblazyscript_prelude.so\n");
                 exit(1);
        }
               int saved_run_main = g_run_main;
               g_run_main         = 0; // -e は最終値を出力
               if (g_trace_dump_path && g_trace_dump_path[0])
          lstrace_begin_dump(g_trace_dump_path);
        if (g_debug)
          lsprintf(stderr, 0, "DBG: eval(-e) begin\n");
        lsthunk_t* ret = lsprog_eval(prog, tenv);
               if (g_debug)
          lsprintf(stderr, 0, "DBG: eval(-e) end ret=%p\n", (void*)ret);
        if (ret != NULL) {
                 if (g_debug) {
                   const char* rt = "?";
                   if (lsthunk_is_err(ret))
              rt = "<bottom>";
            else
              switch (lsthunk_get_type(ret)) {
              case LSTTYPE_ALGE:
                rt = "alge";
                break;
              case LSTTYPE_APPL:
                rt = "appl";
                break;
              case LSTTYPE_LAMBDA:
                rt = "lambda";
                break;
              case LSTTYPE_REF:
                rt = "ref";
                break;
              case LSTTYPE_INT:
                rt = "int";
                break;
              case LSTTYPE_STR:
                rt = "str";
                break;
              case LSTTYPE_SYMBOL:
                rt = "symbol";
                break;
              case LSTTYPE_BUILTIN:
                rt = "builtin";
                break;
              case LSTTYPE_CHOICE:
                rt = "choice";
                break;
              case LSTTYPE_BOTTOM:
                rt = "bottom";
                break;
              }
            lsprintf(stderr, 0, "DBG: eval(-e) ret-type=%s\n", rt);
          }
                 if (lsthunk_is_err(ret)) {
                   lsprintf(stderr, 0, "E: ");
                   lsthunk_print(stderr, LSPREC_LOWEST, 0, ret);
                   if (g_lstrace_table && g_trace_stack_depth > 0)
              lstrace_print_stack(stderr, g_trace_stack_depth);
            lsprintf(stderr, 0, "\n");
          } else {
                   if (g_debug)
              lsprintf(stderr, 0, "DBG: print ret begin\n");
            lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
                   lsprintf(stdout, 0, "\n");
                   if (g_debug)
              lsprintf(stderr, 0, "DBG: print ret end\n");
          }
        }
               g_run_main = saved_run_main;
               if (g_trace_dump_path && g_trace_dump_path[0])
          lstrace_end_dump();
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
           case 1002: // --init <file>
      g_init_file = optarg;
      break;
           case 2000: // --trace-map <file>
      trace_map_path = optarg;
      break;
           case 2001: // --trace-stack-depth <n>
      g_trace_stack_depth = atoi(optarg);
      if (g_trace_stack_depth < 0)
        g_trace_stack_depth = 0;
      break;
           case 2002: // --trace-dump <file>
      g_trace_dump_path = optarg;
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
      printf("      --run-main          run entry function instead of printing top-level value "
             "(off)\n");
      printf("      --entry <name>      set entry function name (default: main)\n");
      printf("      --init <file>   load and evaluate an init LazyScript before user code (thunk "
             "path)\n");
      printf("      --trace-map <file>   load sourcemap JSONL for runtime trace printing (exp)\n");
      printf("      --trace-stack-depth <n>  print up to N frames on error (default: 1)\n");
      printf("      --trace-dump <file>  write JSONL sourcemap while evaluating (exp)\n");
      printf("  -h, --help      display this help and exit\n");
      printf("  -v, --version   output version information and exit\n");
      printf("\nDefault prelude: plugin-only (CLI -p / LAZYSCRIPT_PRELUDE_SO / auto-discover).\n");
      printf("\nEnvironment:\n  LAZYSCRIPT_PRELUDE_SO  path to prelude plugin .so (used if -p not "
             "set)\n");
      printf("  LAZYSCRIPT_PRELUDE_PATH search paths (:) to find liblazyscript_prelude.so when SO "
             "not set\n");
      printf("  LAZYSCRIPT_SUGAR_NS     namespace used for ~~sym sugar (if -n not set)\n");
      printf("  LAZYSCRIPT_INIT         path to init LazyScript (used if --init not set)\n");
      printf("  LAZYSCRIPT_TRACE_MAP    path to sourcemap JSONL (used if --trace-map not set)\n");
      printf(
          "  LAZYSCRIPT_TRACE_STACK_DEPTH  depth to print (used if --trace-stack-depth not set)\n");
      printf("  LAZYSCRIPT_TRACE_DUMP   path to write JSONL (used if --trace-dump not set)\n");
      exit(0);
    case 'v':
      printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
      exit(0);
    default:
      exit(1);
    }
  }

  // Load trace map if configured via CLI or environment
  if (trace_map_path == NULL) {
    const char* env_map = getenv("LAZYSCRIPT_TRACE_MAP");
    if (env_map && env_map[0])
      trace_map_path = env_map;
  }
  if (trace_map_path && trace_map_path[0]) {
    if (lstrace_load_jsonl(trace_map_path) != 0) {
      lsprintf(stderr, 0, "W: failed to load trace map: %s\n", trace_map_path);
    }
  }
  // Load stack depth and dump path from env if not set via CLI
  if (g_trace_stack_depth == 1) {
    const char* env_depth = getenv("LAZYSCRIPT_TRACE_STACK_DEPTH");
    if (env_depth && env_depth[0]) {
      int d = atoi(env_depth);
      if (d >= 0)
        g_trace_stack_depth = d;
    }
  }
  if (g_trace_dump_path == NULL) {
    const char* env_dump = getenv("LAZYSCRIPT_TRACE_DUMP");
    if (env_dump && env_dump[0])
      g_trace_dump_path = env_dump;
  }
  for (int i = optind; i < argc; i++) {
    const char* filename = argv[i];
    if (strcmp(filename, "-") == 0)
      filename = "/dev/stdin";
    const lsprog_t* prog = lsparse_file(filename);
    if (prog != NULL) {
      if (g_debug) {
        lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
      }
      // strict-effects discipline is enforced at runtime; no prevalidation
      lstenv_t* tenv = lstenv_new(NULL);
      if (!ls_try_load_prelude_plugin(tenv, prelude_so)) {
        lsprintf(stderr, 0,
                 "E: prelude: plugin not found or failed to load; set --prelude-so or install "
                 "liblazyscript_prelude.so\n");
        exit(1);
      }
      // Plugin registers prelude dispatchers; no host-side MUX
      // Evaluate init script (if any) into the same environment
      ls_maybe_eval_init(tenv);
      if (g_trace_dump_path && g_trace_dump_path[0])
        lstrace_begin_dump(g_trace_dump_path);
      lsthunk_t* ret = lsprog_eval(prog, tenv);
      if (ret != NULL && !ls_maybe_run_entry(tenv)) {
        if (lsthunk_is_err(ret)) {
          lsprintf(stderr, 0, "E: ");
          lsthunk_print(stderr, LSPREC_LOWEST, 0, ret);
          if (g_lstrace_table && g_trace_stack_depth > 0)
            lstrace_print_stack(stderr, g_trace_stack_depth);
          lsprintf(stderr, 0, "\n");
        } else {
          lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
          lsprintf(stdout, 0, "\n");
        }
      }
      if (g_trace_dump_path && g_trace_dump_path[0])
        lstrace_end_dump();
    }
  }
  // Cleanup
  lstrace_free();
  return 0;
}
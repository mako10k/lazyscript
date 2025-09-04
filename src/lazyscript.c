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
// #include <dlfcn.h> // removed: prelude plugin loading path deleted
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
// run-main/entry support removed: always evaluate and print top-level value
static int         g_trace_stack_depth = 1;      // default: print top frame only
static const char* g_trace_dump_path =
    NULL; // optional: where to emit JSONL in thunk creation order
static int         g_exit_zero_on_error = 0; // default: non-zero exit on error
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
  // Current filename is already set in lsscan_new; no push for top-level
     // Apply sugar namespace if configured
     if (g_sugar_ns == NULL)
    g_sugar_ns = getenv("LAZYSCRIPT_SUGAR_NS");
  if (g_sugar_ns && g_sugar_ns[0])
    lsscan_set_sugar_ns(lsscan, g_sugar_ns);
  int             ret  = yyparse(yyscanner);
     const lsprog_t* prog = NULL;
     if (ret == 0 && !lsscan_has_error(lsscan)) {
       prog = lsscan_get_prog(lsscan);
     } else {
       // On any scanner/parser error, ensure we do not proceed to evaluation
       prog = NULL;
     }
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

// ls_maybe_run_entry: removed

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
// include は字句レイヤーへ移行したため、ホスト側の include ビルトインは廃止

// seq is implemented in builtins/seq.c

// arithmetic builtins are in builtins/arith.c
lsthunk_t* lsbuiltin_lt(lssize_t argc, lsthunk_t* const* args, void* data);

// Prelude plugin path removed: prelude is bound as a value by evaluating lib/Prelude.ls

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

// --- Ephemeral namespaces for Prelude.ls evaluation ---
// Helpers to build symbol and builtin thunks
static inline lsthunk_t* _mk_dotsym(const char* name) {
  // name should include leading '.' (e.g., ".require")
  return lsthunk_new_symbol(lsstr_cstr(name));
}

// Forward declarations from ns.c and require.c
lsthunk_t*  lsbuiltin_prelude_require(lssize_t argc, lsthunk_t* const* args, void* data);

// internal.requireOpt: wrap require into Option-like result (Some () | None)
static lsthunk_t* _internal_requireOpt(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lstenv_t* tenv = (lstenv_t*)data;
  if (!ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: requireOpt: effect used in pure context (enable seq/chain)\n");
    return NULL;
  }
  if (!tenv)
    return ls_make_err("requireOpt: no env");
  lsthunk_t* ret = lsbuiltin_prelude_require(1, args, tenv);
  if (ret == NULL)
    return NULL;
  if (lsthunk_is_err(ret)) {
    // None
    return lsthunk_new_ealge(lsealge_new(lsstr_cstr("None"), 0, NULL), NULL);
  }
  // Some ()
  const lsexpr_t* some_arg     = lsexpr_new_alge(lsealge_new(lsstr_cstr("()"), 0, NULL));
  const lsexpr_t* some_args[1] = { some_arg };
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("Some"), 1, some_args), NULL);
}

// NSLIT core registration
extern lsthunk_t* lsbuiltin_nslit(lssize_t argc, lsthunk_t* const* args, void* data);
extern void       ls_register_nslit_eval(lsthunk_t* (*fn)(lssize_t, lsthunk_t* const*, void*),
                                         void* data);

// Build an ephemeral "internal" namespace value for Prelude.ls evaluation
static lsthunk_t* _build_internal_ns(lstenv_t* tenv) {
  // Pairs: key, value ...
  const int      pairs = 2;
  lsthunk_t**    argv  = (lsthunk_t**)lsmalloc(sizeof(lsthunk_t*) * (size_t)(pairs * 2));
  int            i     = 0;
  // .require
  argv[i++] = _mk_dotsym(".require");
  argv[i++] = lsthunk_new_builtin_attr(lsstr_cstr("internal.require"), 1,
                                       lsbuiltin_prelude_require, tenv,
                                       LSBATTR_EFFECT | LSBATTR_ENV_READ);
  // include/import/withImport は廃止
  // .requireOpt
  argv[i++] = _mk_dotsym(".requireOpt");
  argv[i++] = lsthunk_new_builtin_attr(lsstr_cstr("internal.requireOpt"), 1,
                                       _internal_requireOpt, tenv,
                                       LSBATTR_EFFECT | LSBATTR_ENV_READ);
  lsthunk_t* ns = lsbuiltin_nslit(pairs * 2, argv, NULL);
  lsfree(argv);
  return ns;
}

int        main(int argc, char** argv) {
         // Ensure Boehm GC is initialized early, before any allocations in flex/bison
  // scanner/parser (yylex_init may allocate and trigger GC lazy init otherwise).
  // If libc allocator is requested, skip GC_init to avoid unnecessary GC setup
  // and potential environment-specific issues.
  const char* _ls_use_libc = getenv("LAZYSCRIPT_USE_LIBC_ALLOC");
  if (!(_ls_use_libc && _ls_use_libc[0] && _ls_use_libc[0] != '0')) {
           GC_init();
  }
  // prelude plugin path removed
  int           kind_warn      = 1;    // (kept for future, no coreir)
  int           kind_error     = 0;    // (kept for future, no coreir)
  const char*   trace_map_path = NULL; // optional sourcemap for runtime tracing
  int           exit_status    = 0;    // accumulate non-zero on any error
  // Register core NSLIT evaluator for AST-level namespace literals
  ls_register_nslit_eval(lsbuiltin_nslit, NULL);

  // Env default for exit behavior (CLI overrides)
  {
    const char* ez = getenv("LAZYSCRIPT_EXIT_ZERO_ON_ERROR");
    if (ez && ez[0] && ez[0] != '0')
      g_exit_zero_on_error = 1;
  }
  struct option longopts[]     = {
               { "eval", required_argument, NULL, 'e' },
               { "prelude-so", required_argument, NULL, 'p' },
               { "sugar-namespace", required_argument, NULL, 'n' },
               { "strict-effects", no_argument, NULL, 's' },
               { "init", required_argument, NULL, 1002 },
               { "trace-map", required_argument, NULL, 2000 },
               { "trace-stack-depth", required_argument, NULL, 2001 },
               { "trace-dump", required_argument, NULL, 2002 },
               { "exit-zero-on-error", no_argument, NULL, 3001 },
               { "debug", no_argument, NULL, 'd' },
               { "help", no_argument, NULL, 'h' },
               { "version", no_argument, NULL, 'v' },
               { NULL, 0, NULL, 0 },
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "e:n:sdhv", longopts, NULL)) != -1) {
           switch (opt) {
           case 'e': {
             // Evaluate one-line program
      const lsprog_t* prog = lsparse_string("<cmd>", optarg);
  if (prog != NULL) {
               if (g_debug)
          lsprog_print(stdout, LSPREC_LOWEST, 0, prog);
  lstenv_t* tenv = lstenv_new(NULL);
               // Evaluate lib/Prelude.ls to bind value-prelude, mirroring file-mode path
               {
                 if (g_debug)
                   lsprintf(stderr, 0, "DBG: prelude: parse begin (lib/Prelude.ls)\n");
                 const lsprog_t* prelude_prog = lsparse_file_nullable("lib/Prelude.ls");
                 if (g_debug)
                   lsprintf(stderr, 0, "DBG: prelude: parse %s\n", prelude_prog ? "ok" : "NULL");
                 if (prelude_prog) {
                   lstenv_t* pe = lstenv_new(tenv);
                   // Bind ~builtins: load core builtins namespace once (dlopen is effectful)
                   {
                     if (g_debug)
                       lsprintf(stderr, 0, "DBG: prelude: load builtins(core) begin\n");
                     lsthunk_t* carg     = lsthunk_new_str(lsstr_cstr("core"));
                     lsthunk_t* cargv[1] = { carg };
                     if (ls_effects_get_strict())
                       ls_effects_begin();
                     lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, cargv, pe);
                     if (ls_effects_get_strict())
                       ls_effects_end();
                     if (g_debug) {
                       const char* ct = core_ns ? (lsthunk_is_err(core_ns) ? "<bottom>" : "ok") : "NULL";
                       lsprintf(stderr, 0, "DBG: prelude: load builtins(core) done: %s\n", ct);
                     }
                     if (core_ns)
                       lstenv_put_value(pe, lsstr_cstr("builtins"), core_ns);
                   }
                   // Bind ~internal namespace
                   {
                     if (g_debug)
                       lsprintf(stderr, 0, "DBG: prelude: build ~internal begin\n");
                     lsthunk_t* internal_ns = _build_internal_ns(pe);
                     if (g_debug)
                       lsprintf(stderr, 0, "DBG: prelude: build ~internal %s\n", internal_ns ? "ok" : "NULL");
                     if (internal_ns)
                       lstenv_put_value(pe, lsstr_cstr("internal"), internal_ns);
                   }
                   if (g_debug)
                     lsprintf(stderr, 0, "DBG: prelude: eval begin\n");
                   lsthunk_t* pv = lsprog_eval(prelude_prog, pe);
                   if (g_debug)
                     lsprintf(stderr, 0, "DBG: prelude: eval end pv=%p\n", (void*)pv);
                   if (pv) {
                     // Bind as value: prelude = <record>
                     lstenv_put_value(tenv, lsstr_cstr("prelude"), pv);
                   }
                 }
               }
               // -e は常に最終値を出力
               // Honor env for trace dump in -e path as well
               if (g_trace_dump_path == NULL || g_trace_dump_path[0] == '\0') {
                 const char* env_dump = getenv("LAZYSCRIPT_TRACE_DUMP");
                 if (env_dump && env_dump[0])
                   g_trace_dump_path = env_dump;
               }
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
              exit_status = 1;
            } else {
              if (g_debug)
                lsprintf(stderr, 0, "DBG: print ret begin\n");
              lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
              lsprintf(stdout, 0, "\n");
              if (g_debug)
                lsprintf(stderr, 0, "DBG: print ret end\n");
            }
          }
               // no-op
               if (g_trace_dump_path && g_trace_dump_path[0])
                 lstrace_end_dump();
             }
  // When prog is NULL (parse/scan error), skip evaluation silently; yyerror already printed
  else {
    exit_status = 1; // treat parse/scan error as failure
  }
             break;
    }
           
           case 'n':
      g_sugar_ns = optarg;
      break;
           case 's':
      ls_effects_set_strict(1);
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
      case 3001: // --exit-zero-on-error
    g_exit_zero_on_error = 1;
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
      printf("  -n, --sugar-namespace <ns>  set namespace for ~~sym sugar (default: prelude)\n");
      printf("  -s, --strict-effects  enforce effect discipline (seq/chain required)\n");
      printf("      --init <file>   load and evaluate an init LazyScript before user code (thunk "
             "path)\n");
      printf("      --trace-map <file>   load sourcemap JSONL for runtime trace printing (exp)\n");
      printf("      --trace-stack-depth <n>  print up to N frames on error (default: 1)\n");
      printf("      --trace-dump <file>  write JSONL sourcemap while evaluating (exp)\n");
  printf("      --exit-zero-on-error  keep legacy exit status 0 even on errors (off)\n");
      printf("  -h, --help      display this help and exit\n");
      printf("  -v, --version   output version information and exit\n");
  printf("\nPrelude: evaluated from lib/Prelude.ls and bound as a value (~prelude).\n");
  printf("\nEnvironment:\n");
      printf("  LAZYSCRIPT_SUGAR_NS     namespace used for ~~sym sugar (if -n not set)\n");
      printf("  LAZYSCRIPT_INIT         path to init LazyScript (used if --init not set)\n");
      printf("  LAZYSCRIPT_TRACE_MAP    path to sourcemap JSONL (used if --trace-map not set)\n");
      printf(
          "  LAZYSCRIPT_TRACE_STACK_DEPTH  depth to print (used if --trace-stack-depth not set)\n");
      printf("  LAZYSCRIPT_TRACE_DUMP   path to write JSONL (used if --trace-dump not set)\n");
  printf("  LAZYSCRIPT_EXIT_ZERO_ON_ERROR  when set (non-empty, not '0'), exit 0 even on errors\n");
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
  // Allow enabling trace dump via environment if --trace-dump is not specified
  if (g_trace_dump_path == NULL || g_trace_dump_path[0] == '\0') {
    const char* env_dump = getenv("LAZYSCRIPT_TRACE_DUMP");
    if (env_dump && env_dump[0])
      g_trace_dump_path = env_dump;
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
      // Plugin registers prelude dispatchers; now evaluate lib/Prelude.ls in a
      // child environment with ephemeral ~builtins and ~internal, then bind the
      // resulting value under name 'prelude' in the user environment.
      {
        if (g_debug)
          lsprintf(stderr, 0, "DBG: [file] prelude: parse begin (lib/Prelude.ls)\n");
        const lsprog_t* prelude_prog = lsparse_file_nullable("lib/Prelude.ls");
        if (g_debug)
          lsprintf(stderr, 0, "DBG: [file] prelude: parse %s\n", prelude_prog ? "ok" : "NULL");
        if (prelude_prog) {
          lstenv_t* pe = lstenv_new(tenv);
          // Bind ~builtins: load core builtins namespace once (dlopen is effectful)
          {
            if (g_debug)
              lsprintf(stderr, 0, "DBG: [file] prelude: load builtins(core) begin\n");
            lsthunk_t* carg     = lsthunk_new_str(lsstr_cstr("core"));
            lsthunk_t* cargv[1] = { carg };
            if (ls_effects_get_strict())
              ls_effects_begin();
            lsthunk_t* core_ns = lsbuiltin_prelude_builtin(1, cargv, pe);
            if (ls_effects_get_strict())
              ls_effects_end();
            if (g_debug) {
              const char* ct = core_ns ? (lsthunk_is_err(core_ns) ? "<bottom>" : "ok") : "NULL";
              lsprintf(stderr, 0, "DBG: [file] prelude: load builtins(core) done: %s\n", ct);
            }
            if (core_ns)
              lstenv_put_value(pe, lsstr_cstr("builtins"), core_ns);
          }
          // Bind ~internal namespace
          {
            if (g_debug)
              lsprintf(stderr, 0, "DBG: [file] prelude: build ~internal begin\n");
            lsthunk_t* internal_ns = _build_internal_ns(pe);
            if (g_debug)
              lsprintf(stderr, 0, "DBG: [file] prelude: build ~internal %s\n", internal_ns ? "ok" : "NULL");
            if (internal_ns)
              lstenv_put_value(pe, lsstr_cstr("internal"), internal_ns);
          }
          if (g_debug)
            lsprintf(stderr, 0, "DBG: [file] prelude: eval begin\n");
          lsthunk_t* pv = lsprog_eval(prelude_prog, pe);
          if (g_debug)
            lsprintf(stderr, 0, "DBG: [file] prelude: eval end pv=%p\n", (void*)pv);
          if (pv) {
            // Bind as value: prelude = <record>
            lstenv_put_value(tenv, lsstr_cstr("prelude"), pv);
          }
        }
      }
      if (g_trace_dump_path && g_trace_dump_path[0])
        lstrace_begin_dump(g_trace_dump_path);
  lsthunk_t* ret = lsprog_eval(prog, tenv);
  if (ret != NULL) {
        if (lsthunk_is_err(ret)) {
          lsprintf(stderr, 0, "E: ");
          lsthunk_print(stderr, LSPREC_LOWEST, 0, ret);
          if (g_lstrace_table && g_trace_stack_depth > 0)
            lstrace_print_stack(stderr, g_trace_stack_depth);
          lsprintf(stderr, 0, "\n");
          exit_status = 1;
        } else {
          lsthunk_print(stdout, LSPREC_LOWEST, 0, ret);
          lsprintf(stdout, 0, "\n");
        }
      }
      if (g_trace_dump_path && g_trace_dump_path[0])
        lstrace_end_dump();
  }
  // else: parse/scan error already reported via yyerror; proceed to next file
  else {
    exit_status = 1; // parse/scan error
  }
  }
  // Cleanup
  lstrace_free();
  if (g_exit_zero_on_error)
    return 0;
  return exit_status;
}
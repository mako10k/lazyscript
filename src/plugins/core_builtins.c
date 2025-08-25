#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"
#include "runtime/effects.h"
#include "runtime/unit.h"
#include "runtime/error.h"

// Reuse existing builtin implementations from the main binary
// (the main program is linked with -export-dynamic so symbols are visible).
// Prototypes from runtime/builtin.h would also work, but keep minimal deps here.
lsthunk_t* lsbuiltin_dump(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_to_string(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_print(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_seq(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_add(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_sub(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_lt(lssize_t argc, lsthunk_t* const* args, void* data);
// namespace helpers from main binary
lsthunk_t* lsbuiltin_ns_members(lssize_t argc, lsthunk_t* const* args, void* data);
lsthunk_t* lsbuiltin_prelude_ns_self(lssize_t argc, lsthunk_t* const* args, void* data);
// Equality: provide a local minimal eq to avoid coupling to prelude module
static lsthunk_t* cb_eq(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  lsthunk_t* a = lsthunk_eval0(args[0]); if (!a) return NULL;
  lsthunk_t* b = lsthunk_eval0(args[1]); if (!b) return NULL;
  if (lsthunk_get_type(a) == LSTTYPE_INT && lsthunk_get_type(b) == LSTTYPE_INT) {
    int eq = lsint_eq(lsthunk_get_int(a), lsthunk_get_int(b));
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  if (lsthunk_get_type(a) == LSTTYPE_SYMBOL && lsthunk_get_type(b) == LSTTYPE_SYMBOL) {
    const lsstr_t* sa = lsthunk_get_symbol(a);
    const lsstr_t* sb = lsthunk_get_symbol(b);
    int eq = (lsstrcmp(sa, sb) == 0);
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  if (lsthunk_get_type(a) == LSTTYPE_ALGE && lsthunk_get_argc(a) == 0 &&
      lsthunk_get_type(b) == LSTTYPE_ALGE && lsthunk_get_argc(b) == 0) {
    const lsstr_t* ca = lsthunk_get_constr(a);
    const lsstr_t* cb = lsthunk_get_constr(b);
    int eq = (lsstrcmp(ca, cb) == 0);
    return lsthunk_new_ealge(lsealge_new(eq ? lsstr_cstr("true") : lsstr_cstr("false"), 0, NULL), NULL);
  }
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("false"), 0, NULL), NULL);
}

// println: print value (using existing print) then add a newline; return unit
static lsthunk_t* cb_println(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  // Delegate to print for conversion/side-effect guard
  lsthunk_t* r = lsbuiltin_print(1, args, NULL);
  if (!r) return NULL; // error or effect guard violation propagated
  // Append newline only if print succeeded
  lsprintf(stdout, 0, "\n");
  // Return unit value
  return lsthunk_new_ealge(lsealge_new(lsstr_cstr("()"), 0, NULL), NULL);
}

// chain: enable effects, evaluate action, then pass unit to continuation
static lsthunk_t* cb_chain(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  ls_effects_begin();
  lsthunk_t* action = ls_eval_arg(args[0], "chain: action");
  ls_effects_end();
  if (lsthunk_is_err(action)) return action;
  if (!action) return ls_make_err("chain: action eval");
  lsthunk_t* unit = ls_make_unit();
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &unit);
}

// bind: enable effects while evaluating value, then pass to continuation
static lsthunk_t* cb_bind(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data;
  ls_effects_begin();
  lsthunk_t* val = ls_eval_arg(args[0], "bind: value");
  ls_effects_end();
  if (lsthunk_is_err(val)) return val;
  if (!val) return ls_make_err("bind: value eval");
  lsthunk_t* cont = args[1];
  return lsthunk_eval(cont, 1, &val);
}

// return: identity (no evaluation)
static lsthunk_t* cb_return(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)data; return args[0];
}

// Minimal builtin loader ABI (duplicated locally to avoid header dependency)
typedef struct ls_builtin_entry {
  const char* name;
  int         arity;
  lsthunk_t* (*fn)(lssize_t, lsthunk_t* const*, void*);
  void*       data;
} ls_builtin_entry_t;

typedef struct ls_builtin_module {
  int                        abi_version;   // must be 1
  const char*                module_name;   // e.g., "core"
  const char*                module_version;// optional
  const ls_builtin_entry_t*  entries;       // array
  int                        entry_count;   // count
  void*                      user_data;     // optional
} ls_builtin_module_t;

static const ls_builtin_entry_t CORE_ENTRIES[] = {
  { "dump",   1, lsbuiltin_dump,        NULL },
  { "to_str", 1, lsbuiltin_to_string,   NULL },
  { "print",  1, lsbuiltin_print,       NULL },
  { "println",1, cb_println,            NULL },
  { "chain",  2, cb_chain,              NULL },
  { "bind",   2, cb_bind,               NULL },
  { "return", 1, cb_return,             NULL },
  { "seq",    2, lsbuiltin_seq,         (void*)0 },
  { "seqc",   2, lsbuiltin_seq,         (void*)1 },
  { "add",    2, lsbuiltin_add,         NULL },
  { "sub",    2, lsbuiltin_sub,         NULL },
  { "lt",     2, lsbuiltin_lt,          NULL },
  { "eq",     2, cb_eq,                 NULL },
  { "nsMembers", 1, lsbuiltin_ns_members, NULL },
  { "nsSelf",    0, lsbuiltin_prelude_ns_self, NULL },
};

static ls_builtin_module_t CORE_MOD = {
  1,
  "core",
  PACKAGE_VERSION,
  CORE_ENTRIES,
  (int)(sizeof(CORE_ENTRIES) / sizeof(CORE_ENTRIES[0])),
  NULL
};

__attribute__((visibility("default")))
ls_builtin_module_t* lazyscript_builtin_init(void) {
  return &CORE_MOD;
}

#include "thunk/thunk.h"
#include "expr/ealge.h"
#include "common/str.h"
#include "common/int.h"
#include <assert.h>

// Simple functions: add1(x) = x + 1, hello() = "hello"
static lsthunk_t* demo_add1(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)data; assert(argc == 1);
  lsthunk_t* v = lsthunk_eval0(args[0]); if (!v) return NULL;
  if (lsthunk_get_type(v) != LSTTYPE_INT) return NULL;
  int n = lsint_get(lsthunk_get_int(v));
  return lsthunk_new_int(lsint_new(n + 1));
}

static lsthunk_t* demo_hello(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc; (void)args; (void)data;
  return lsthunk_new_str(lsstr_cstr("hello"));
}

// Minimal module descriptor for builtin loader
typedef struct ls_builtin_entry {
  const char* name;
  int         arity;
  lsthunk_t* (*fn)(lssize_t, lsthunk_t* const*, void*);
  void*       data;
} ls_builtin_entry_t;

typedef struct ls_builtin_module {
  int                        abi_version;   // must be 1
  const char*                module_name;   // e.g., "demo"
  const char*                module_version;// optional
  const ls_builtin_entry_t*  entries;       // array
  int                        entry_count;   // count
  void*                      user_data;     // optional
} ls_builtin_module_t;

static const ls_builtin_entry_t DEMO_ENTRIES[] = {
  { "add1", 1, demo_add1, NULL },
  { "hello", 0, demo_hello, NULL },
};

static ls_builtin_module_t DEMO_MOD = {
  1,
  "demo",
  "0.1",
  DEMO_ENTRIES,
  2,
  NULL
};

__attribute__((visibility("default")))
ls_builtin_module_t* lazyscript_builtin_init(void) {
  return &DEMO_MOD;
}

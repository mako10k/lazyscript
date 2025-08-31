#ifndef LAZYSCRIPT_THUNK_BIN_H
#define LAZYSCRIPT_THUNK_BIN_H

#include <stdio.h>
#include "lstypes.h" // for lssize_t

#ifdef __cplusplus
extern "C" {
#endif

// Magic 'LSTB' and version (v0.1)
#define LSTB_MAGIC 0x4C535442u
#define LSTB_VERSION_MAJOR 1u
#define LSTB_VERSION_MINOR 1u

// File-level flags
#define LSTB_F_STORE_WHNF   (1u << 0)
#define LSTB_F_STORE_TRACE  (1u << 1)
#define LSTB_F_STORE_LOCS   (1u << 2)
#define LSTB_F_STORE_TYPES  (1u << 3)

// Thunk kinds (table entry kinds)
typedef enum lstb_kind {
  LSTB_KIND_ALGE   = 0,
  LSTB_KIND_APPL   = 1,
  LSTB_KIND_CHOICE = 2,
  LSTB_KIND_LAMBDA = 3,
  LSTB_KIND_REF    = 4,
  LSTB_KIND_INT    = 5,
  LSTB_KIND_STR    = 6,
  LSTB_KIND_SYMBOL = 7,
  LSTB_KIND_BUILTIN= 8,
  LSTB_KIND_BOTTOM = 9
} lstb_kind_t;

// Choice operator sub-kind
// 1='|' (lambda-choice), 2='||' (expr-choice), 3='^|' (catch)
typedef enum lstb_choice_kind {
  LSTB_CK_LAMBDA = 1,
  LSTB_CK_EXPR   = 2,
  LSTB_CK_CATCH  = 3
} lstb_choice_kind_t;

// Entry flags (per-thunk)
#define LSTB_EF_WHNF (1u << 0)
#define LSTB_EF_HAS_TYPE (1u << 1)

// TYPE_POOL entry kinds (reservation)
typedef enum lstb_type_kind {
  LSTB_TK_STRING_NAME = 0,  // STRING_POOL id
  LSTB_TK_SYMBOL_NAME = 1,  // SYMBOL_POOL id
  LSTB_TK_OPAQUE_BLOB = 2   // opaque binary blob (reserved)
} lstb_type_kind_t;

// Forward decls (opaque types from runtime)
typedef struct lsthunk lsthunk_t;
typedef struct lstenv  lstenv_t;

// Serialize runtime thunk graph rooted at `roots[0..rootc)` to `fp`.
// Returns 0 on success, negative on error.
int lstb_write(FILE* fp, lsthunk_t* const* roots, lssize_t rootc, unsigned file_flags);

// Deserialize thunk graph from `fp`.
// - out_roots: returns newly allocated array of root pointers (size=*out_rootc)
// - prelude_env: optional environment for resolving external/builtin refs (may be NULL)
// Returns 0 on success, negative on error.
int lstb_read(FILE* fp, lsthunk_t*** out_roots, lssize_t* out_rootc, unsigned* out_file_flags,
              lstenv_t* prelude_env);

#ifdef __cplusplus
}
#endif

#endif // LAZYSCRIPT_THUNK_BIN_H

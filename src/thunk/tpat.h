#pragma once

// Thunk-side pattern representation and helpers

typedef struct lstpat lstpat_t;

// Forward declarations to avoid circular includes
typedef struct lsthunk              lsthunk_t;
typedef struct lstenv               lstenv_t;
typedef struct lstref_target_origin lstref_target_origin_t;

#include <stdio.h>
#include "lstypes.h"
#include "common/int.h"
#include "common/str.h"
#include "common/ref.h"
#include "pat/pat.h"
#include "pat/palge.h"
#include "pat/pas.h"

// Constructors and converters
// Convert AST pattern (lspat_t) into thunk pattern (lstpat_t)
lstpat_t* lstpat_new_pat(const lspat_t* pat, lstenv_t* tenv, lstref_target_origin_t* origin);

// Create a thunk REF pattern directly
lstpat_t* lstpat_new_ref(const lsref_t* ref);

// Raw constructors for runtime loaders (do not depend on AST)
// These mirror internal representations and are safe for materializers.
lstpat_t* lstpat_new_alge_raw(const lsstr_t* constr, lssize_t argc, lstpat_t* const* args);
lstpat_t* lstpat_new_as_raw(lstpat_t* ref_pat, lstpat_t* inner_pat);
lstpat_t* lstpat_new_int_raw(const lsint_t* val);
lstpat_t* lstpat_new_str_raw(const lsstr_t* val);
lstpat_t* lstpat_new_wild_raw(void);
lstpat_t* lstpat_new_or_raw(lstpat_t* left, lstpat_t* right);
lstpat_t* lstpat_new_caret_raw(lstpat_t* inner);

// Accessors
lsptype_t lstpat_get_type(const lstpat_t* pat);

// ALGE
const lsstr_t*   lstpat_get_constr(const lstpat_t* pat);
lssize_t         lstpat_get_argc(const lstpat_t* pat);
lstpat_t* const* lstpat_get_args(const lstpat_t* pat);

// AS pattern
lstpat_t* lstpat_get_ref(const lstpat_t* pat);
lstpat_t* lstpat_get_aspattern(const lstpat_t* pat);

// INT / STR
const lsint_t* lstpat_get_int(const lstpat_t* pat);
const lsstr_t* lstpat_get_str(const lstpat_t* pat);
int            lstpat_is_wild(const lstpat_t* pat);

// REF pattern accessors
// Get the referenced name (lsref->name) from a REF pattern
const lsstr_t* lstpat_get_refname(const lstpat_t* pat);
// Low-level: get the lsref_t* from a REF pattern (debug/serialization helpers)
const lsref_t* lstpat_get_refref(const lstpat_t* pat);

// OR pattern accessors (thunk-side)
lstpat_t* lstpat_get_or_left(const lstpat_t* pat);
lstpat_t* lstpat_get_or_right(const lstpat_t* pat);

// Caret pattern (^pat) — only matches Bottom values, then matches inner pattern
lstpat_t* lstpat_new_caret(const lspat_t* inner_ast, lstenv_t* tenv,
                           lstref_target_origin_t* origin);
lstpat_t* lstpat_get_caret_inner(const lstpat_t* pat);

// REF bound management
void       lstpat_set_refbound(lstpat_t* pat, lsthunk_t* thunk);
lsthunk_t* lstpat_get_refbound(const lstpat_t* pat);

// Utilities
lstpat_t* lstpat_clone(const lstpat_t* pat);
void      lstpat_print(FILE* fp, lsprec_t prec, int indent, const lstpat_t* pat);

// Clear all REF bindings inside the pattern (for lambda reuse)
void lstpat_clear_binds(lstpat_t* pat);

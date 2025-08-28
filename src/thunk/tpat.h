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

// OR pattern accessors (thunk-side)
lstpat_t* lstpat_get_or_left(const lstpat_t* pat);
lstpat_t* lstpat_get_or_right(const lstpat_t* pat);

// Caret pattern (^pat) — only matches Bottom values, then matches inner pattern
lstpat_t* lstpat_new_caret(const lspat_t* inner_ast, lstenv_t* tenv, lstref_target_origin_t* origin);
lstpat_t* lstpat_get_caret_inner(const lstpat_t* pat);

// REF bound management
void       lstpat_set_refbound(lstpat_t* pat, lsthunk_t* thunk);
lsthunk_t* lstpat_get_refbound(const lstpat_t* pat);

// Utilities
lstpat_t* lstpat_clone(const lstpat_t* pat);
void      lstpat_print(FILE* fp, lsprec_t prec, int indent, const lstpat_t* pat);

// Clear all REF bindings inside the pattern (for lambda reuse)
void lstpat_clear_binds(lstpat_t* pat);

#pragma once

typedef struct lsthunk              lsthunk_t;
typedef struct lstalge              lstalge_t;
typedef struct lstappl              lstappl_t;
typedef struct lstchoice            lstchoice_t;
typedef struct lstbind              lstbind_t;
typedef struct lstlambda            lstlambda_t;
typedef struct lstref               lstref_t;
typedef struct lstref_target        lstref_target_t;
typedef struct lstref_target_origin lstref_target_origin_t;
typedef struct lstbuiltin           lstbuiltin_t;

typedef enum lstrtype { LSTRTYPE_BIND, LSTRTYPE_LAMBDA, LSTRTYPE_BUILTIN } lstrtype_t;

#include "lstypes.h"

typedef lsthunk_t* (*lstbuiltin_func_t)(lssize_t, lsthunk_t* const*, void*);

// Builtin attribute flags (bitmask)
//  - PURE (default): no side-effects, no env access
//  - EFFECT: performs side-effects (requires effects to be allowed)
//  - ENV_READ: reads from environment via data pointer
//  - ENV_WRITE: mutates environment via data pointer
typedef enum lsbuiltin_attr {
  LSBATTR_PURE      = 0,
  LSBATTR_EFFECT    = 1 << 0,
  LSBATTR_ENV_READ  = 1 << 1,
  LSBATTR_ENV_WRITE = 1 << 2,
} lsbuiltin_attr_t;

#include "common/int.h"
#include "common/str.h"
#include "expr/ealge.h"
#include "misc/prog.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pat.h"
#include "thunk/tpat.h"
#include "thunk/tenv.h"

typedef enum lsttype {
  LSTTYPE_ALGE,
  LSTTYPE_APPL,
  LSTTYPE_CHOICE,
  LSTTYPE_INT,
  LSTTYPE_LAMBDA,
  LSTTYPE_REF,
  LSTTYPE_STR,
  // First-class symbol literal (e.g., .name)
  LSTTYPE_SYMBOL,
  LSTTYPE_BUILTIN,
  // Dedicated bottom value carrying message, location, and related thunks
  LSTTYPE_BOTTOM
} lsttype_t;

/**
 * Create a new thunk for an algebraic data type
 * @param ealge The algebraic expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_ealge(const lsealge_t* ealge, lstenv_t* tenv);

/**
 * Create a new thunk for an application data type
 * @param eappl The application expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_eappl(const lseappl_t* eappl, lstenv_t* tenv);

/**
 * Create a new thunk for a choice data type
 * @param echoice The choice expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_echoice(const lsechoice_t* echoice, lstenv_t* tenv);

/**
 * Create a new thunk for a closure data type
 * @param eclosure The closure expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_eclosure(const lseclosure_t* eclosure, lstenv_t* tenv);

/**
 * Create a new thunk for an integer data type
 * @param intval The integer value
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_int(const lsint_t* intval);

/**
 * Create a new thunk for a lambda data type
 * @param elambda The lambda expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_elambda(const lselambda_t* elambda, lstenv_t* tenv);

/**
 * Create a new thunk for a reference data type
 * @param ref The reference value
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_ref(const lsref_t* ref, lstenv_t* tenv);

/**
 * Create a new thunk for a string data type
 * @param strval The string value
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_str(const lsstr_t* strval);

/**
 * Create a new thunk for a symbol literal (e.g., .name)
 * @param sym The symbol identifier (should include leading dot as parsed)
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_symbol(const lsstr_t* sym);

/**
 * Create a new thunk for a builtin data type
 * @param name The name of the builtin (only for debugging)
 * @param arity The number of arguments
 * @param func The function
 * @param data The data
 * @return The new thunk
 */
lsthunk_t* lsthunk_new_builtin(const lsstr_t* name, lssize_t arity, lstbuiltin_func_t func,
                               void* data);

// Create a new builtin thunk with attributes
lsthunk_t* lsthunk_new_builtin_attr(const lsstr_t* name, lssize_t arity, lstbuiltin_func_t func,
                                    void* data, lsbuiltin_attr_t attr);

// Builtin helpers (introspection)
int              lsthunk_is_builtin(const lsthunk_t* thunk);
lstbuiltin_func_t lsthunk_get_builtin_func(const lsthunk_t* thunk);
void*            lsthunk_get_builtin_data(const lsthunk_t* thunk);
const lsstr_t*   lsthunk_get_builtin_name(const lsthunk_t* thunk);

// Bottom (⊥) constructors and accessors
// Create a bottom value with message and optional related thunks (argc may be 0)
lsthunk_t* lsthunk_new_bottom(const char* message, lsloc_t loc, lssize_t argc, lsthunk_t* const* args);
// Convenience: bottom with current pending/unknown location
lsthunk_t* lsthunk_bottom_here(const char* message);
// Predicates and getters
int         lsthunk_is_bottom(const lsthunk_t* thunk);
const char* lsthunk_bottom_get_message(const lsthunk_t* thunk);
lsloc_t     lsthunk_bottom_get_loc(const lsthunk_t* thunk);
lssize_t    lsthunk_bottom_get_argc(const lsthunk_t* thunk);
lsthunk_t* const* lsthunk_bottom_get_args(const lsthunk_t* thunk);
// Merge/accumulate two bottoms: concatenates messages with "; " and appends args; chooses earlier location
lsthunk_t* lsthunk_bottom_merge(lsthunk_t* a, lsthunk_t* b);

// Internal-friendly constructors for two-phase wiring (used by loaders/materializers)
// Allocate an ALGE thunk with space for argc args; arguments are left unset for later wiring
lsthunk_t* lsthunk_alloc_alge(const lsstr_t* constr, lssize_t argc);
// Set the i-th argument on an ALGE thunk allocated via lsthunk_alloc_alge
void        lsthunk_set_alge_arg(lsthunk_t* thunk, lssize_t idx, lsthunk_t* arg);
// Allocate a BOTTOM thunk with message and reserved related slots; args left NULL for later wiring
lsthunk_t*  lsthunk_alloc_bottom(const char* message, lsloc_t loc, lssize_t argc);
// Set the i-th related thunk on a BOTTOM allocated via lsthunk_alloc_bottom
void        lsthunk_set_bottom_related(lsthunk_t* thunk, lssize_t idx, lsthunk_t* arg);

/**
 * Create a new thunk for an expression
 * @param expr The expression
 * @param tenv The environment
 * @return The new thunk (If null, the expr has errors)
 */
lsthunk_t* lsthunk_new_expr(const lsexpr_t* expr, lstenv_t* tenv);

/**
 * Get the type of a thunk
 * @param thunk The thunk
 * @return The type of the thunk
 */
lsttype_t lsthunk_get_type(const lsthunk_t* thunk);

/**
 * Get the algebraic constructor of a thunk
 * @param thunk The thunk
 * @return The algebraic constructor
 */
const lsstr_t* lsthunk_get_constr(const lsthunk_t* thunk);


/**
 * Get the arguments of a thunk
 * @param thunk The thunk
 * @return The arguments
 */
lssize_t lsthunk_get_argc(const lsthunk_t* thunk);

/**
 * Get the arguments of a thunk
 * @param thunk The thunk
 * @return The arguments
 */
lsthunk_t* const* lsthunk_get_args(const lsthunk_t* thunk);

/**
 * Get the integer value of a thunk
 * @param thunk The thunk
 * @return The integer value
 */
const lsint_t* lsthunk_get_int(const lsthunk_t* thunk);

/**
 * Get the string value of a thunk
 * @param thunk The thunk
 * @return The string value
 */
const lsstr_t*    lsthunk_get_str(const lsthunk_t* thunk);

/**
 * Get the symbol value of a thunk
 * @param thunk The thunk (must be of type LSTTYPE_SYMBOL)
 * @return The symbol identifier string (includes leading dot)
 */
const lsstr_t*    lsthunk_get_symbol(const lsthunk_t* thunk);

lssize_t          lsthunk_get_argc(const lsthunk_t* thunk);

lsthunk_t* const* lsthunk_get_args(const lsthunk_t* thunk);

// removed: lsthunk_get_left (unused)

// removed: lsthunk_get_right (unused)

lstpat_t*         lsthunk_get_param(const lsthunk_t* thunk);

lsthunk_t*        lsthunk_get_body(const lsthunk_t* thunk);

// removed: lsthunk_get_ref_target (unused)

// Substitute occurrences of lambda parameter inside a thunk with its bound value
lsthunk_t* lsthunk_subst_param(lsthunk_t* thunk, lstpat_t* param);

/**
 * Associate a thunk with an algebraic pattern
 * @param thunk The thunk
 * @param pat The algebraic pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_alge(lsthunk_t* thunk, lstpat_t* tpat);

/**
 * Associate a thunk with an as pattern
 * @param thunk The thunk
 * @param pat The as pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pas(lsthunk_t* thunk, lstpat_t* tpat);

/**
 * Associate a thunk with a reference pattern
 * @param thunk The thunk
 * @param pat The reference pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_ref(lsthunk_t* thunk, lstpat_t* tpat);

/**
 * Associate a thunk with a pattern
 * @param thunk The thunk
 * @param pat The pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pat(lsthunk_t* thunk, lstpat_t* tpat);

/**
 * Evaluate a thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t* lsthunk_eval0(lsthunk_t* thunk);

/**
 * Apply a thunk to a list of arguments
 * @param func The function thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The result of the application
 */
lsthunk_t*       lsthunk_eval(lsthunk_t* func, lssize_t argc, lsthunk_t* const* args);

lstref_target_t* lstref_target_new(lstref_target_origin_t* origin, lstpat_t* pat);

// Accessor for environments/targets: retrieve the pattern associated to target
lstpat_t*               lstref_target_get_pat(lstref_target_t* target);

lstref_target_origin_t* lstref_target_origin_new_builtin(const lsstr_t* name, lssize_t arity,
                                                         lstbuiltin_func_t func, void* data);

lsthunk_t*              lsprog_eval(const lsprog_t* prog, lstenv_t* tenv);

void                    lsthunk_print(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk);

void                    lsthunk_dprint(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk);
// Deep print: recursively evaluate substructures while printing (used by to_string)
void                    lsthunk_deep_print(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk);

// For now, cloning a thunk returns the same pointer (immutable semantics).
// If thunk mutability is introduced, replace with a deep copy.
lsthunk_t* lsthunk_clone(lsthunk_t* thunk);

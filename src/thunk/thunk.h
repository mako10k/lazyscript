#pragma once

typedef struct lsthunk lsthunk_t;
typedef struct lstalge lstalge_t;
typedef struct lstappl lstappl_t;
typedef struct lstchoice lstchoice_t;
typedef struct lstbind lstbind_t;
typedef struct lstlambda lstlambda_t;
typedef struct lstref lstref_t;
typedef struct lstref_target lstref_target_t;
typedef struct lstref_target_origin lstref_target_origin_t;
typedef struct lstinternal lstinternal_t;

typedef enum lstrtype { LSTRTYPE_BIND, LSTRTYPE_LAMBDA } lstrtype_t;

#include "common/int.h"
#include "common/str.h"
#include "expr/ealge.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pat.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"

typedef enum lsttype {
  LSTTYPE_ALGE,
  LSTTYPE_APPL,
  LSTTYPE_CHOICE,
  LSTTYPE_INT,
  LSTTYPE_LAMBDA,
  LSTTYPE_REF,
  LSTTYPE_STR,
  LSTTYPE_INTERNAL
} lsttype_t;

/**
 * Create a new thunk for an algebraic data type
 * @param ealge The algebraic expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_ealge(const lsealge_t *ealge, lstenv_t *tenv);

/**
 * Create a new thunk for an application data type
 * @param tappl The application expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_eappl(const lseappl_t *eappl, lstenv_t *tenv);

/**
 * Create a new thunk for a choice data type
 * @param echoice The choice expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_echoice(const lsechoice_t *echoice, lstenv_t *tenv);

/**
 * Create a new thunk for a closure data type
 * @param eclosure The closure expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_eclosure(const lseclosure_t *eclosure, lstenv_t *tenv);

/**
 * Create a new thunk for an integer data type
 * @param intval The integer value
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_int(const lsint_t *intval);

/**
 * Create a new thunk for a lambda data type
 * @param elambda The lambda expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_elambda(const lselambda_t *elambda, lstenv_t *tenv);

/**
 * Create a new thunk for a reference data type
 * @param ref The reference value
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_ref(const lsref_t *ref, lstenv_t *tenv);

/**
 * Create a new thunk for a string data type
 * @param strval The string value
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_str(const lsstr_t *strval);

/**
 * Create a new thunk for an expression
 * @param expr The expression
 * @param tenv The environment
 * @return The new thunk (If null, the expr has errors)
 */
lsthunk_t *lsthunk_new_expr(const lsexpr_t *expr, lstenv_t *tenv);

/**
 * Get the type of a thunk
 * @param thunk The thunk
 * @return The type of the thunk
 */
lsttype_t lsthunk_get_type(const lsthunk_t *thunk);

/**
 * Get the algebraic constructor of a thunk
 * @param thunk The thunk
 * @return The algebraic constructor
 */
const lsstr_t *lsthunk_get_constr(const lsthunk_t *thunk);

/**
 * Get the function of a thunk
 * @param thunk The thunk
 * @return The function
 */
lsthunk_t *lsthunk_get_func(const lsthunk_t *thunk);

/**
 * Get the arguments of a thunk
 * @param thunk The thunk
 * @return The arguments
 */
lssize_t lsthunk_get_argc(const lsthunk_t *thunk);

/**
 * Get the arguments of a thunk
 * @param thunk The thunk
 * @return The arguments
 */
lsthunk_t *const *lsthunk_get_args(const lsthunk_t *thunk);

/**
 * Get the integer value of a thunk
 * @param thunk The thunk
 * @return The integer value
 */
const lsint_t *lsthunk_get_int(const lsthunk_t *thunk);

/**
 * Get the string value of a thunk
 * @param thunk The thunk
 * @return The string value
 */
const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk);

lssize_t lsthunk_get_argc(const lsthunk_t *thunk);

lsthunk_t *const *lsthunk_get_args(const lsthunk_t *thunk);

lsthunk_t *lsthunk_get_left(const lsthunk_t *thunk);

lsthunk_t *lsthunk_get_right(const lsthunk_t *thunk);

lstpat_t *lsthunk_get_param(const lsthunk_t *thunk);

lsthunk_t *lsthunk_get_body(const lsthunk_t *thunk);

lstref_target_t *lsthunk_get_ref_target(const lsthunk_t *thunk);

/**
 * Associate a thunk with an algebraic pattern
 * @param thunk The thunk
 * @param tpat The algebraic pattern
 * @return result
 */
lsmres_t lsthunk_match_alge(lsthunk_t *thunk, lstpat_t *tpat);

/**
 * Associate a thunk with an as pattern
 * @param thunk The thunk
 * @param tpat The as pattern
 * @return result
 */
lsmres_t lsthunk_match_pas(lsthunk_t *thunk, lstpat_t *tpat);

/**
 * Associate a thunk with a reference pattern
 * @param thunk The thunk
 * @param tpat The reference pattern
 * @return result
 */
lsmres_t lsthunk_match_ref(lsthunk_t *thunk, lstpat_t *tpat);

/**
 * Associate a thunk with a pattern
 * @param thunk The thunk
 * @param pat The pattern
 * @return result
 */
lsmres_t lsthunk_match_pat(lsthunk_t *thunk, lstpat_t *tpat);

/**
 * Evaluate a thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval0(lsthunk_t *thunk);

/**
 * Apply a thunk to a list of arguments
 * @param func The function thunk
 * @param args The arguments
 * @return The result of the application
 */
lsthunk_t *lsthunk_eval(lsthunk_t *func, lssize_t argc, lsthunk_t *const *args);

lstref_target_t *lstref_target_new(lstref_target_origin_t *origin, lstpat_t *tpat);
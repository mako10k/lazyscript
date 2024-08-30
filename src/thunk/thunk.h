#pragma once

typedef struct lsthunk lsthunk_t;

#include "common/int.h"
#include "common/str.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pat.h"
#include "thunk/talge.h"
#include "thunk/tappl.h"
#include "thunk/tchoice.h"
#include "thunk/tlambda.h"
#include "thunk/tref.h"

typedef enum lsttype {
  LSTTYPE_ALGE,
  LSTTYPE_APPL,
  LSTTYPE_CHOICE,
  LSTTYPE_INT,
  LSTTYPE_LAMBDA,
  LSTTYPE_REF,
  LSTTYPE_STR,
} lsttype_t;

/**
 * Create a new thunk for an algebraic data type
 * @param ealge The algebraic expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_alge(const lsealge_t *ealge, lstenv_t *tenv);

/**
 * Create a new thunk for an application data type
 * @param tappl The application expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_appl(const lseappl_t *eappl, lstenv_t *tenv);

/**
 * Create a new thunk for a choice data type
 * @param echoice The choice expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_choice(const lsechoice_t *echoice, lstenv_t *tenv);

/**
 * Create a new thunk for a closure data type
 * @param eclosure The closure expression
 * @param tenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_closure(const lseclosure *eclosure, lstenv_t *tenv);

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
lsthunk_t *lsthunk_new_lambda(const lselambda_t *elambda, lstenv_t *tenv);

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
 * Get the algebraic value of a thunk
 * @param thunk The thunk
 * @return The algebraic value
 */
lstalge_t *lsthunk_get_alge(const lsthunk_t *thunk);

/**
 * Get the application value of a thunk
 * @param thunk The thunk
 * @return The application value
 */
lstappl_t *lsthunk_get_appl(const lsthunk_t *thunk);

/**
 * Get the choice value of a thunk
 * @param thunk The thunk
 * @return The choice value
 */
lstchoice_t *lsthunk_get_choice(const lsthunk_t *thunk);

/**
 * Get the integer value of a thunk
 * @param thunk The thunk
 * @return The integer value
 */
const lsint_t *lsthunk_get_int(const lsthunk_t *thunk);

/**
 * Get the lambda value of a thunk
 * @param thunk The thunk
 * @return The lambda value
 */
lstlambda_t *lsthunk_get_lambda(const lsthunk_t *thunk);

/**
 * Get the reference value of a thunk
 * @param thunk The thunk
 * @return The reference value
 */
lstref_t *lsthunk_get_ref(const lsthunk_t *thunk);

/**
 * Get the string value of a thunk
 * @param thunk The thunk
 * @return The string value
 */
const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk);

/**
 * Associate a thunk with an algebraic pattern
 * @param thunk The thunk
 * @param alge The algebraic pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_palge(lsthunk_t *thunk, const lspalge_t *alge,
                             lstenv_t *tenv);

/**
 * Associate a thunk with an as pattern
 * @param thunk The thunk
 * @param pas The as pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pas(lsthunk_t *thunk, const lspas_t *pas,
                           lstenv_t *tenv);

/**
 * Associate a thunk with a reference pattern
 * @param thunk The thunk
 * @param ref The reference pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pref(lsthunk_t *thunk, const lsref_t *ref,
                            lstenv_t *tenv);

/**
 * Associate a thunk with a pattern
 * @param thunk The thunk
 * @param pat The pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pat(lsthunk_t *thunk, const lspat_t *pat,
                           lstenv_t *tenv);

/**
 * Evaluate a thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval(lsthunk_t *thunk);

/**
 * Apply a thunk to a list of arguments
 * @param func The function thunk
 * @param args The arguments
 * @return The result of the application
 */
lsthunk_t *lsthunk_apply(lsthunk_t *func, const lstlist_t *args);
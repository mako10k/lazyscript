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
typedef struct lstbuiltin lstbuiltin_t;

typedef enum lstrtype {
  LSTRTYPE_BIND,
  LSTRTYPE_LAMBDA,
  LSTRTYPE_THUNK
} lstrtype_t;

#include "lstypes.h"

typedef lsthunk_t *(*lstbuiltin_func_t)(lssize_t, lsthunk_t *const *, void *);

#include "common/int.h"
#include "common/str.h"
#include "expr/ealge.h"
#include "misc/prog.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pat.h"
#include "thunk/eenv.h"
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
  LSTTYPE_BUILTIN
} lsttype_t;

#define lsapi_beta lsapi_nn12 lsapi_wur

lsthunk_t *lsthunk_new_alge(const lsstr_t *constr, lssize_t argc,
                            lsthunk_t *const *args);

lsthunk_t *lsthunk_new_appl(lsthunk_t *func, lssize_t argc,
                            lsthunk_t *const *args);

lsthunk_t *lsthunk_new_choice(lsthunk_t *left, lsthunk_t *right);

lsthunk_t *lsthunk_new_lambda(lstpat_t *param, lsthunk_t *body);

lsthunk_t *lsthunk_new_ref(const lsref_t *ref, lseref_target_t *target);

/**
 * Beta reduction for thunk and make a copy of thunk and replace the thunk with
 * the new thunk
 * @param thunk The thunk
 * @param tenv The environment
 */
lsapi_beta lsthunk_t *lsthunk_beta(lsthunk_t *thunk, lstenv_t *tenv);

/**
 * Beta reduction for algebraic thunk
 * @param thunk The thunk
 * @param tenv The environment
 * @return The new thunk
 */
lsapi_beta lsthunk_t *lsthunk_beta_alge(lsthunk_t *thunk, lstenv_t *tenv);

/**
 * Beta reduction for application thunk
 * @param thunk The thunk
 * @param tenv The environment
 * @return The new thunk
 */
lsapi_beta lsthunk_t *lsthunk_beta_appl(lsthunk_t *thunk, lstenv_t *tenv);

/**
 * Beta reduction for lambda thunk
 * @param thunk The thunk
 * @param tenv The environment
 * @return The new thunk
 */
lsapi_beta lsthunk_t *lsthunk_beta_lambda(lsthunk_t *thunk, lstenv_t *tenv);

/**
 * Create new thunk from algebraic expression
 * @param ealge The algebraic expression
 * @param eenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_ealge(const lsealge_t *ealge, lseenv_t *eenv);

/**
 * Create a new thunk from an application expression
 * @param eappl The application expression
 * @param eenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_eappl(const lseappl_t *eappl, lseenv_t *eenv);

/**
 * Create a new thunk from a choice expression
 * @param echoice The choice expression
 * @param eenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_echoice(const lsechoice_t *echoice, lseenv_t *eenv);

/**
 * Create a new thunk from a closure expression
 * @param eclosure The closure expression
 * @param eenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_eclosure(const lseclosure_t *eclosure, lseenv_t *eenv);

/**
 * Create a new thunk from a lambda expression
 * @param elambda The lambda expression
 * @param eenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_elambda(const lselambda_t *elambda, lseenv_t *eenv);

/**
 * Create a new thunk from a reference expression
 * @param ref The reference expression
 * @param eenv The environment
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_eref(const lsref_t *ref, lseenv_t *eenv);

/**
 * Create a new thunk for an integer data type
 * @param intval The integer value
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_int(const lsint_t *intval);

/**
 * Create a new thunk for a string data type
 * @param strval The string value
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_str(const lsstr_t *strval);

/**
 * Create a new thunk for a builtin data type
 * @param name The name of the builtin (only for debugging)
 * @param arity The number of arguments
 * @param func The function
 * @param data The data
 * @return The new thunk
 */
lsthunk_t *lsthunk_new_builtin(const lsstr_t *name, lssize_t arity,
                               lstbuiltin_func_t func, void *data);

/**
 * Create a new thunk for an expression
 * @param expr The expression
 * @param eenv The environment
 * @return The new thunk (If null, the expr has errors)
 */
lsthunk_t *lsthunk_new_expr(const lsexpr_t *expr, lseenv_t *eenv);

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
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_alge(lsthunk_t *thunk, lstpat_t *tpat, lstenv_t *tenv);

/**
 * Associate a thunk with an as pattern
 * @param thunk The thunk
 * @param tpat The as pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pas(lsthunk_t *thunk, lstpat_t *tpat, lstenv_t *tenv);

/**
 * Associate a thunk with a reference pattern
 * @param thunk The thunk
 * @param tpat The reference pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_ref(lsthunk_t *thunk, lstpat_t *tpat, lstenv_t *tenv);

/**
 * Associate a thunk with a pattern
 * @param thunk The thunk
 * @param tpat The pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pat(lsthunk_t *thunk, lstpat_t *tpat, lstenv_t *tenv);

/**
 * Evaluate a thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval0(lsthunk_t *thunk);

/**
 * Apply a thunk to a list of arguments
 * @param func The function thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The result of the application
 */
lsthunk_t *lsthunk_eval(lsthunk_t *func, lssize_t argc, lsthunk_t *const *args);

/**
 * Evaluate an algebratic thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_alge(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args);

/**
 * Evaluate an application thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_appl(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args);

/**
 * Evaluate a lambda thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_lambda(lsthunk_t *thunk, lssize_t argc,
                               lsthunk_t *const *args);

/**
 * Evaluate a builtin function thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_builtin(lsthunk_t *thunk, lssize_t argc,
                                lsthunk_t *const *args);
/**
 * Evaluate a reference thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_ref(lsthunk_t *thunk, lssize_t argc,
                            lsthunk_t *const *args);

/**
 * Evaluate a choice thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lstref_target_t *lstref_target_new(lstref_target_origin_t *origin,
                                   lstpat_t *tpat);

lstref_target_origin_t *lstref_target_origin_new_builtin(const lsstr_t *name,
                                                         lssize_t arity,
                                                         lstbuiltin_func_t func,
                                                         void *data);

lsthunk_t *lsprog_eval(const lsprog_t *prog, lstenv_t *tenv);

void lsthunk_print(FILE *fp, lsprec_t prec, int indent, lsthunk_t *thunk);

void lsthunk_dprint(FILE *fp, lsprec_t prec, int indent, lsthunk_t *thunk);

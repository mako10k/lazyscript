#pragma once

typedef struct lsthunk lsthunk_t;
typedef struct lstalge lstalge_t;
typedef struct lstappl lstappl_t;
typedef struct lstbeta lstbeta_t;
typedef struct lstbind lstbind_t;
typedef struct lstbindref lstbindref_t;
typedef struct lstbuiltin lstbuiltin_t;
typedef struct lstchoice lstchoice_t;
typedef struct lstlambda lstlambda_t;
typedef struct lstparamref lstparamref_t;
typedef struct lstthunkref lstthunkref_t;

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
  LSTTYPE_BETA,
  LSTTYPE_BINDREF,
  LSTTYPE_BUILTIN,
  LSTTYPE_CHOICE,
  LSTTYPE_INT,
  LSTTYPE_LAMBDA,
  LSTTYPE_PARAMREF,
  LSTTYPE_THUNKREF,
  LSTTYPE_STR,
} lsttype_t;

#define lsapi_beta lsapi_nn12 lsapi_wur
#define lsapi_new_thunk_beta lsapi_nn12 lsapi_wur
#define lsapi_new_thunk_alge lsapi_nn1 lsapi_wur
#define lsapi_new_thunk_appl lsapi_nn1 lsapi_wur
#define lsapi_new_thunk_bindref lsapi_nn12 lsapi_wur
#define lsapi_new_thunk_builtin lsapi_nn13 lsapi_wur
#define lsapi_new_thunk_choice lsapi_wur
#define lsapi_new_thunk_int lsapi_wur
#define lsapi_new_thunk_lambda lsapi_nn12 lsapi_wur
#define lsapi_new_thunk_paramref lsapi_nn1 lsapi_wur
#define lsapi_new_thunk_str lsapi_wur
#define lsapi_new_thunk_thunkref lsapi_wur

/**
 * Create a new thunk for beta reduction (but deferred)
 * @param env The environment
 * @param thunk The thunk
 * @return The new thunk
 */
lsapi_new_thunk_beta lsthunk_t *lsthunk_new_beta(const lstenv_t *env,
                                                 const lsthunk_t *thunk);

/**
 * Create a new thunk for an algebraic data type
 * @param constr The constructor
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk
 */
lsapi_new_thunk_alge lsthunk_t *
lsthunk_new_alge(const lsstr_t *constr, lssize_t argc, lsthunk_t *const *args);

/**
 * Create a new thunk for an application
 * @param func The function
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk
 */
lsapi_new_thunk_appl lsthunk_t *lsthunk_new_appl(lsthunk_t *func, lssize_t argc,
                                                 lsthunk_t *const *args);

/**
 * Create a new thunk for a bind reference
 * @param bind The bind
 * @param ref The reference
 * @param bound The bound thunk
 * @return The new thunk
 */
lsapi_new_thunk_bindref lsthunk_t *
lsthunk_new_bindref(lstbind_t *bind, const lsref_t *ref, lsthunk_t *bound);

/**
 * Create a new thunk for a builtin data type
 * @param name The name of the builtin (only for debugging)
 * @param arity The number of arguments
 * @param func The function
 * @param data The data
 * @return The new thunk
 */
lsapi_new_thunk_builtin lsthunk_t *lsthunk_new_builtin(const lsstr_t *name,
                                                       lssize_t arity,
                                                       lstbuiltin_func_t func,
                                                       void *data);

/**
 * Create a new thunk for a choice
 * @param left The left thunk
 * @param right The right thunk
 * @return The new thunk
 */
lsapi_new_thunk_choice lsthunk_t *lsthunk_new_choice(lsthunk_t *left,
                                                     lsthunk_t *right);

/**
 * Create a new thunk for an integer data type
 * @param intval The integer value
 * @return The new thunk
 */
lsapi_new_thunk_int lsthunk_t *lsthunk_new_int(const lsint_t *intval);

/**
 * Create a new thunk for a lambda
 * @param param The parameter
 * @param body The body
 * @return The new thunk
 */
lsapi_new_thunk_lambda lsthunk_t *lsthunk_new_lambda(lstpat_t *param,
                                                     lsthunk_t *body);

/**
 * Create a new thunk for a parameter reference
 * @param param The parameter
 * @param bound The bound thunk
 * @return The new thunk
 */
lsapi_new_thunk_paramref lsthunk_t *
lsthunk_new_paramref(lsthunk_t *lambda, const lsref_t *ref, lsthunk_t *bound);

/**
 * Create a new thunk for a string data type
 * @param strval The string value
 * @return The new thunk
 */
lsapi_new_thunk_str lsthunk_t *lsthunk_new_str(const lsstr_t *strval);

/**
 * Create a new thunk for a thunk reference
 * @param ref The reference
 * @return The new thunk
 */
lsapi_new_thunk_thunkref lsthunk_t *lsthunk_new_thunkref(const lsref_t *ref,
                                                         lsthunk_t *bound);

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
lsapi_get lsapi_pure lsttype_t lsthunk_get_type(const lsthunk_t *thunk);

/**
 * Check if a thunk is a list thunk
 * @param thunk The thunk
 * @return True if the thunk is a list thunk
 */
lsapi_get lsapi_pure lsbool_t lsthunk_is_list(const lsthunk_t *thunk);

/**
 * Check if a thunk is a cons thunk
 * @param thunk The thunk
 * @return True if the thunk is a cons thunk
 */
lsapi_get lsapi_pure lsbool_t lsthunk_is_cons(const lsthunk_t *thunk);

/**
 * Check if a thunk is a nil list thunk
 * @param thunk The thunk
 * @return True if the thunk is a nil list thunk
 */
lsapi_get lsapi_pure lsbool_t lsthunk_is_nil(const lsthunk_t *thunk);

/**
 * Get the nil list thunk
 * @return The nil list thunk
 */
lsapi_pure const lsthunk_t *lsthunk_get_nil(void);

/**
 * Create a new list thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new list thunk
 */
lsthunk_t *lsthunk_new_list(lssize_t argc, lsthunk_t *const *args);

/**
 * Create a new cons thunk
 * @param head The head
 * @param tail The tail
 * @return The new cons thunk
 */
lsapi_nn12 lsthunk_t *lsthunk_new_cons(lsthunk_t *head, lsthunk_t *tail);

/**
 * Get the algebraic constructor of a thunk
 * @param thunk The thunk
 * @return The algebraic constructor
 */
lsapi_get lsapi_pure const lsstr_t *lsthunk_get_constr(const lsthunk_t *thunk);

/**
 * Get the function of a thunk
 * @param thunk The thunk
 * @return The function
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_func(const lsthunk_t *thunk);

/**
 * Get the arguments of a thunk
 * @param thunk The thunk
 * @return The arguments
 */
lsapi_get lsapi_pure lssize_t lsthunk_get_argc(const lsthunk_t *thunk);

/**
 * Get the arguments of a thunk
 * @param thunk The thunk
 * @return The arguments
 */
lsapi_get lsapi_pure lsthunk_t *const *lsthunk_get_args(const lsthunk_t *thunk);

/**
 * Get the integer value of a thunk
 * @param thunk The thunk
 * @return The integer value
 */
lsapi_get lsapi_pure const lsint_t *lsthunk_get_int(const lsthunk_t *thunk);

/**
 * Get the string value of a thunk
 * @param thunk The thunk
 * @return The string value
 */
lsapi_get lsapi_pure const lsstr_t *lsthunk_get_str(const lsthunk_t *thunk);

/**
 * Get the left thunk of a choice thunk
 * @param thunk The thunk
 * @return The left thunk
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_left(const lsthunk_t *thunk);

/**
 * Get the right thunk of a choice thunk
 * @param thunk The thunk
 * @return The right thunk
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_right(const lsthunk_t *thunk);

/**
 * Get the parameter of a lambda thunk
 * @param thunk The thunk
 * @return The parameter
 */
lsapi_get lsapi_pure lstpat_t *lsthunk_get_param(const lsthunk_t *thunk);

/**
 * Get the body of a lambda thunk
 * @param thunk The thunk
 * @return The body
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_body(const lsthunk_t *thunk);

/**
 * Get the bind of a bind reference thunk
 * @param thunk The thunk
 * @return The bind
 */
lsapi_get lsapi_pure lstbind_t *lsthunk_get_bind(const lsthunk_t *thunk);

/**
 * Get the lambda of a parameter reference thunk
 * @param thunk The thunk
 * @return The bind
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_lambda(const lsthunk_t *thunk);

/**
 * Get the reference of a parameter reference thunk
 * @param thunk The thunk
 * @return The reference
 */
lsapi_get lsapi_pure const lsref_t *lsthunk_get_ref(const lsthunk_t *thunk);

/**
 * Get the bound data of a bind reference thunk
 * @param thunk The thunk
 * @return The bound
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_bound(const lsthunk_t *thunk);

/**
 * Get the environment of the beta thunk
 * @param thunk The thunk
 * @return The environment
 */
lsapi_get lsapi_pure lstenv_t *lsthunk_get_env(const lsthunk_t *thunk);

/**
 * Get the thunk of the beta thunk
 * @param thunk The thunk
 * @return The thunk
 */
lsapi_get lsapi_pure lsthunk_t *lsthunk_get_thunk(const lsthunk_t *thunk);

/**
 * Get the name of a builtin thunk
 * @param thunk The thunk
 * @return The bound
 */
lsapi_get lsapi_pure const lsstr_t *
lsthunk_get_builtin_name(const lsthunk_t *thunk);

/**
 * Get the arity of a builtin thunk
 * @param thunk The thunk
 * @return The bound
 */
lsapi_get lsapi_pure lssize_t lsthunk_get_builtin_arity(const lsthunk_t *thunk);

/**
 * Get the function of a builtin thunk
 * @param thunk The thunk
 * @return The bound
 */
lsapi_get lsapi_pure lstbuiltin_func_t
lsthunk_get_builtin_func(const lsthunk_t *thunk);

/**
 * Get the data of a builtin thunk
 * @param thunk The thunk
 * @return The bound
 */
lsapi_get lsapi_pure void *lsthunk_get_builtin_data(const lsthunk_t *thunk);

/**
 * Associate a thunk with an algebraic pattern
 * @param thunk The thunk
 * @param tpat The algebraic pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_palge(lsthunk_t *thunk, const lstpat_t *tpat,
                             lstenv_t *tenv);

/**
 * Associate a thunk with an as pattern
 * @param thunk The thunk
 * @param tpat The as pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pas(lsthunk_t *thunk, const lstpat_t *tpat,
                           lstenv_t *tenv);

/**
 * Associate a thunk with a reference pattern
 * @param thunk The thunk
 * @param tpat The reference pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pref(lsthunk_t *thunk, const lstpat_t *tpat,
                            lstenv_t *tenv);

/**
 * Associate a thunk with a pattern
 * @param thunk The thunk
 * @param tpat The pattern
 * @param tenv The environment
 * @return result
 */
lsmres_t lsthunk_match_pat(lsthunk_t *thunk, const lstpat_t *tpat,
                           lstenv_t *tenv);

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
 * Evaluate a thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval(lsthunk_t *thunk);

/**
 * Apply a thunk to a list of arguments
 * @param func The function thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The result of the application
 */
lsthunk_t *lsthunk_eval_thunk(lsthunk_t *func, lssize_t argc,
                              lsthunk_t *const *args);

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
 * Evaluate a beta thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_beta(lsthunk_t *thunk, lssize_t argc,
                             lsthunk_t *const *args);

/**
 * Evaluate a bind reference thunk to WHNF (Weak Head Normal Form)
 * @param thunk The thunk
 * @param argc The number of arguments
 * @param args The arguments
 * @return The new thunk evaluate to WHNF
 */
lsthunk_t *lsthunk_eval_choice(lsthunk_t *thunk, lssize_t argc,
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

lsthunk_t *lsprog_eval(const lsprog_t *prog, lstenv_t *tenv);

void lsthunk_print(FILE *fp, lsprec_t prec, int indent, lsthunk_t *thunk);

void lsthunk_dprint(FILE *fp, lsprec_t prec, int indent, lsthunk_t *thunk);

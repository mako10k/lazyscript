#pragma once

typedef struct lstenv lstenv_t;
typedef struct lstref_target lstref_target_t;
typedef struct lstref_target_origin lstref_target_origin_t;
typedef enum lstrtype {
  LSTRTYPE_BIND,
  LSTRTYPE_PARAM,
  LSTRTYPE_THUNK
} lstrtype_t;

#include "common/str.h"
#include "thunk/eenv.h"
#include "thunk/thunk.h"

/**
 * Create new environment
 * @param parent The parent environment
 * @return The new environment
 */
lstenv_t *lstenv_new(const lstenv_t *parent);

/**
 * Get target from environment (find target also in ancester environments)
 * @param tenv The environment
 * @param name The name
 * @return The target
 */
lstref_target_t *lstenv_get(const lstenv_t *tenv, const lsstr_t *name);

/**
 * Get target from only current environment
 * @param tenv The environment
 * @param name The name
 * @return The target
 */
lstref_target_t *lstenv_get_self(const lstenv_t *tenv, const lsstr_t *name);

/**
 * Put target into current environment
 * @param tenv The environment
 * @param name The name
 * @param target The target
 */
void lstenv_put(lstenv_t *tenv, const lsstr_t *name, lstref_target_t *target);

/**
 * Increment the number of warnings
 * @param tenv The environment
 */
void lstenv_incr_nwarnings(lstenv_t *tenv);

/**
 * Increment the number of errors
 * @param tenv The environment
 */
void lstenv_incr_nerrors(lstenv_t *tenv);

/**
 * Increment the number of fatals
 * @param tenv The environment
 */
void lstenv_incr_nfatals(lstenv_t *tenv);

/**
 * Get the number of warnings
 * @param tenv The environment
 * @return The number of warnings
 */
lssize_t lstenv_get_nwarnings(const lstenv_t *tenv);

/**
 * Get the number of errors
 * @param tenv The environment
 * @return The number of errors
 */
lssize_t lstenv_get_nerrors(const lstenv_t *tenv);

/**
 * Get the number of fatals
 * @param tenv The environment
 * @return The number of fatals
 */
lssize_t lstenv_get_nfatals(const lstenv_t *tenv);

/**
 * Print environment
 * @param fp The file
 * @param tenv The environment
 */
void lstenv_print(FILE *fp, const lstenv_t *tenv);

/**
 * Put builtin into environment
 * @param tenv The environment
 * @param name The name
 * @param arity The number of arguments
 * @param func The function
 * @param data The data
 */
void lstenv_put_builtin(lstenv_t *tenv, const lsstr_t *name, lssize_t arity,
                        lstbuiltin_func_t func, void *data);


lstrtype_t lstref_target_get_type(lstref_target_t *target);
lsthunk_t *lstref_target_get_thunk(lstref_target_t *target);
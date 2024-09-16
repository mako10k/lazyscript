#pragma once

typedef struct lstenv lstenv_t;

#include "common/str.h"
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
lsthunk_t *lstenv_get(const lstenv_t *tenv, const lsstr_t *name);

/**
 * Get target from only current environment
 * @param tenv The environment
 * @param name The name
 * @return The target
 */
lsthunk_t *lstenv_get_self(const lstenv_t *tenv, const lsstr_t *name);

/**
 * Put target into current environment
 * @param tenv The environment
 * @param name The name
 * @param target The target
 */
void lstenv_put(lstenv_t *tenv, const lsstr_t *name, lsthunk_t *target);

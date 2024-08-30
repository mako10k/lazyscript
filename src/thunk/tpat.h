#pragma once

typedef struct lstpat lstpat_t;
typedef struct lstpalge lstpalge_t;
typedef struct lstpas lstpas_t;
typedef struct lstpref lstpref_t;

#include "expr/expr.h"
#include "pat/pat.h"
#include "thunk/tenv.h"

#define lsapi_tpalge_new lsapi_nn13 lsapi_wur
#define laspi_tas_new lsapi_nn12 lsapi_wur
#define lsapi_tref_new lsapi_nn1 lsapi_wur
#define lsapi_tpat_new lsapi_nn1 lsapi_wur

/**
 * @brief Create a new algebraic pattern.
 *
 * @param constr The constructor of the algebraic pattern.
 * @param argc The number of arguments.
 * @param args The arguments.
 * @return lstpalge_t* The new algebraic pattern.
 */
lsapi_tpalge_new lstpalge_t *lstpalge_new(const lsstr_t *constr, lssize_t argc,
                                          lstpat_t *args[]);

/**
 * @brief Create a new as pattern.
 *
 * @param ref The reference of the as pattern.
 * @param tpat The pattern of the as pattern.
 * @return lstpas_t* The new as pattern.
 */
laspi_tas_new lstpas_t *lstpas_new(const lsref_t *ref, lstpat_t *tpat);

/**
 * @brief Create a new reference pattern.
 *
 * @param ref The reference of the reference pattern.
 * @return lstpref_t* The new reference pattern.
 */
lsapi_tref_new lstpref_t *lstpref_new(const lsref_t *ref);

/**
 * @brief Create a new aglebraic pattern.
 * @param palge The algebraic pattern.
 * @return lstpat_t* The new pattern.
 */
lsapi_tpat_new lstpat_t *lstpat_new_alge(lstpalge_t *tpalge);

/**
 * @brief Create a new as pattern.
 *
 * @param pas The as pattern.
 * @return lstpat_t* The new pattern.
 */
lsapi_tpat_new lstpat_t *lstpat_new_as(lstpas_t *tpas);

/**
 * @brief Create a new integer pattern.
 *
 * @param val The integer value.
 * @return lstpat_t* The new pattern.
 */
lsapi_tpat_new lstpat_t *lstpat_new_int(const lsint_t *val);

/**
 * @brief Create a new reference pattern.
 *
 * @param pref The reference pattern.
 * @return lstpat_t* The new pattern.
 */
lsapi_tpat_new lstpat_t *lstpat_new_ref(lstpref_t *tpref);

/**
 * @brief Create a new string pattern.
 *
 * @param val The string value.
 * @return lstpat_t* The new pattern.
 */
lsapi_tpat_new lstpat_t *lstpat_new_str(const lsstr_t *val);

/**
 * @brief Get the type of the pattern.
 *
 * @param pat The pattern.
 * @return lsptype_t The type of the pattern.
 */
lsapi_get lsptype_t lstpat_get_type(const lstpat_t *tpat);

/**
 * @brief Get the algebraic pattern.
 *
 * @param pat The pattern.
 * @return const lstpalge_t* The algebraic pattern.
 */
lsapi_get const lstpalge_t *lstpat_get_alge(const lstpat_t *tpat);

/**
 * @brief Get the as pattern.
 *
 * @param pat The pattern.
 * @return const lstpas_t* The as pattern.
 */
lsapi_get const lstpas_t *lstpat_get_as(const lstpat_t *tpat);

/**
 * @brief Get the integer value.
 *
 * @param pat The pattern.
 * @return const lsint_t* The integer value.
 */
lsapi_get const lsint_t *lstpat_get_int(const lstpat_t *tpat);

/**
 * @brief Get the reference.
 *
 * @param pat The pattern.
 * @return const lsref_t* The reference.
 */
lsapi_get const lstpref_t *lstpat_get_ref(const lstpat_t *tpat);

/**
 * @brief Get the string value.
 *
 * @param pat The pattern.
 * @return const lsstr_t* The string value.
 */
lsapi_get const lsstr_t *lstpat_get_str(const lstpat_t *tpat);
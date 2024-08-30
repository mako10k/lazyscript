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
#define lsapi_tpat_get lsapi_nn1 lsapi_wur

lsapi_tpalge_new lstpalge_t *lstpalge_new(const lsstr_t *constr, lssize_t argc,
                                          lstpat_t *args[]);

laspi_tas_new lstpas_t *lstpas_new(const lsref_t *ref, lstpat_t *pat);

lsapi_tref_new lstpref_t *lstpref_new(const lsref_t *ref);

lsapi_tpat_new lstpat_t *lstpat_new_alge(lstpalge_t *palge);

lsapi_tpat_new lstpat_t *lstpat_new_as(lstpas_t *pas);

lsapi_tpat_new lstpat_t *lstpat_new_int(const lsint_t *val);

lsapi_tpat_new lstpat_t *lstpat_new_ref(lstpref_t *pref);

lsapi_tpat_new lstpat_t *lstpat_new_str(const lsstr_t *val);

lsapi_tpat_get lsptype_t lstpat_get_type(const lstpat_t *pat);

lsapi_tpat_get const lstpalge_t *lstpat_get_alge(const lstpat_t *pat);

lsapi_tpat_get const lstpas_t *lstpat_get_as(const lstpat_t *pat);

lsapi_tpat_get const lsint_t *lstpat_get_int(const lstpat_t *pat);

lsapi_tpat_get const lstpref_t *lstpat_get_ref(const lstpat_t *pat);

lsapi_tpat_get const lsstr_t *lstpat_get_str(const lstpat_t *pat);
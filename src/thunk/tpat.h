#pragma once

typedef struct lstpat lstpat_t;

#include "common/int.h"
#include "common/str.h"
#include "lstypes.h"
#include "thunk/eenv.h"
#include "thunk/thunk.h"

lstpat_t *lstpat_new_alge(const lsstr_t *constr, lssize_t argc,
                          lstpat_t *const *args);

lstpat_t *lstpat_new_ref(const lsref_t *ref);

lstpat_t *lstpat_new_as(const lsref_t *ref, lstpat_t *aspattern);

lstpat_t *lstpat_new_int(const lsint_t *intval);

lstpat_t *lstpat_new_str(const lsstr_t *strval);

lstpat_t *lstpat_new_pat(const lspat_t *pat, lseenv_t *tenv,
                         lseref_target_origin_t *origin);

lsptype_t lstpat_get_type(const lstpat_t *tpat);

lsapi_get lsapi_pure lsbool_t lstpat_is_list(const lstpat_t *tpat);

lsapi_get lsapi_pure lsbool_t lstpat_is_cons(const lstpat_t *tpat);

lsapi_get lsapi_pure lsbool_t lstpat_is_nil(const lstpat_t *tpat);

lsapi_pure const lstpat_t *lstpat_get_nil(void);

const lsstr_t *lstpat_get_constr(const lstpat_t *tpat);

lssize_t lstpat_get_argc(const lstpat_t *tpat);

lstpat_t *const *lstpat_get_args(const lstpat_t *tpat);

lstpat_t *lstpat_get_asref(const lstpat_t *tpat);

lstpat_t *lstpat_get_aspattern(const lstpat_t *tpat);

const lsref_t *lstpat_get_ref(const lstpat_t *tpat);

const lsint_t *lstpat_get_int(const lstpat_t *tpat);

const lsstr_t *lstpat_get_str(const lstpat_t *tpat);

void lstpat_print(FILE *fp, lsprec_t prec, int indent, const lstpat_t *tpat);
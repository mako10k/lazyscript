#pragma once

typedef struct lstbind lstbind_t;

#include "thunk/thunk.h"

#define lsapi_tbind_new lsapi_nn12 lsapi_wur

lsapi_tbind_new lstbind_t *lstbind_new(const lstpat_t *lhs, lsthunk_t *rhs);

lsapi_get lstpat_t *lstbind_get_lhs(const lstbind_t *bind);
lsapi_get lsthunk_t *lstbind_get_rhs(const lstbind_t *bind);
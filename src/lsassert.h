#pragma once

#include "common/const.h"
#include "common/str.h"
#include "thunk/thunk.h"
#include "thunk/tpat.h"
#include <assert.h>

#define lsassert(cond) assert(cond)

#define lsassert_args(argc, args) lsassert((argc) == 0 || (args) != NULL)
#define lsassert_notreach() lsassert(0)

#define lsassert_talge(thunk) lsassert(lsthunk_get_type(thunk) == LSTTYPE_ALGE)
#define lsassert_tappl(thunk) lsassert(lsthunk_get_type(thunk) == LSTTYPE_APPL)
#define lsassert_tbeta(thunk) lsassert(lsthunk_get_type(thunk) == LSTTYPE_BETA)
#define lsassert_tbindref(thunk)                                               \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_BINDREF)
#define lsassert_tbuiltin(thunk)                                               \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_BUILTIN)
#define lsassert_tchoice(thunk)                                                \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_CHOICE)
#define lsassert_tstr(thunk) lsassert(lsthunk_get_type(thunk) == LSTTYPE_STR)
#define lsassert_tcons(thunk) lsassert(lsthunk_is_cons(thunk))
#define lsassert_tlambda(thunk)                                                \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_LAMBDA)
#define lsassert_tlist(thunk) lsassert(lsthunk_is_list(thunk))
#define lsassert_tnil(thunk) lsassert(lsthunk_is_nil(thunk))
#define lsassert_tcons(thunk) lsassert(lsthunk_is_cons(thunk))
#define lsassert_tbindref(thunk)                                               \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_BINDREF)
#define lsassert_tparamref(thunk)                                              \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_PARAMREF)
#define lsassert_tthunkref(thunk)                                              \
  lsassert(lsthunk_get_type(thunk) == LSTTYPE_THUNKREF)

#define lsassert_tpalge(tpat) lsassert(lstpat_get_type(tpat) == LSPTYPE_ALGE)
#define lsassert_tpas(tpat) lsassert(lstpat_get_type(tpat) == LSPTYPE_AS)
#define lsassert_tpref(tpat) lsassert(lstpat_get_type(tpat) == LSPTYPE_REF)
#define lsassert_tpint(tpat) lsassert(lstpat_get_type(tpat) == LSPTYPE_INT)
#define lsassert_tpstr(tpat) lsassert(lstpat_get_type(tpat) == LSPTYPE_STR)
#define lsassert_tpcons(tpat) lsassert(lstpat_is_cons(tpat))
#define lsassert_tplist(tpat) lsassert(lstpat_is_list(tpat))
#define lsassert_tpnil(tpat) lsassert(lstpat_is_nil(tpat))

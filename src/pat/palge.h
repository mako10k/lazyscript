#pragma once

typedef struct lspalge lspalge_t;

#include "common/array.h"
#include "common/str.h"
#include "lazyscript.h"
#include "pat/pat.h"
#include "pat/plist.h"
#include "thunk/tenv.h"
#include "thunk/thunk.h"

lspalge_t *lspalge_new(const lsstr_t *constr);
void lsexpr_add_args(lspalge_t *alge, lspat_t *arg);
void lsexpr_concat_args(lspalge_t *alge, const lsplist_t *args);
const lsstr_t *lspalge_get_constr(const lspalge_t *alge);
lssize_t lspalge_get_arg_count(const lspalge_t *alge);
lspat_t *lspalge_get_arg(const lspalge_t *alge, int i);
const lsplist_t *lspalge_get_args(const lspalge_t *alge);
void lspalge_print(FILE *fp, int prec, int indent, const lspalge_t *alge);
lspres_t lspalge_prepare(lspalge_t *alge, lseenv_t *env,
                         lserref_wrapper_t *erref);
lsmres_t lspalge_match(lstenv_t *tenv, const lspalge_t *alge, lsthunk_t *thunk);

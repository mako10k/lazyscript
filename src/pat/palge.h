#pragma once

typedef struct lspalge lspalge_t;

#include "common/array.h"
#include "common/str.h"
#include "lazyscript.h"
#include "pat/pat.h"
#include "pat/plist.h"

lspalge_t *lspalge_new(const lsstr_t *constr);
void lsexpr_add_args(lspalge_t *palge, const lspat_t *arg);
void lsexpr_concat_args(lspalge_t *palge, const lsplist_t *args);
const lsstr_t *lspalge_get_constr(const lspalge_t *palge);
lssize_t lspalge_get_arg_count(const lspalge_t *palge);
const lspat_t *lspalge_get_arg(const lspalge_t *palge, int i);
const lsplist_t *lspalge_get_args(const lspalge_t *palge);
void lspalge_print(FILE *fp, int prec, int indent, const lspalge_t *palge);
lspres_t lspalge_prepare(const lspalge_t *palge, lseenv_t *env,
                         const lserref_base_t *erref);

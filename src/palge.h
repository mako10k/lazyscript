#pragma once

typedef struct lspalge lspalge_t;

#include "array.h"
#include "pat.h"
#include "str.h"

lspalge_t *lspalge(const lsstr_t *constr);
void lspalge_push_arg(lspalge_t *alge, lspat_t *arg);
void lspalge_push_args(lspalge_t *alge, const lsarray_t *args);
const lsstr_t *lspalge_get_constr(const lspalge_t *alge);
unsigned int lspalge_get_argc(const lspalge_t *alge);
lspat_t *lspalge_get_arg(const lspalge_t *alge, int i);
void lspalge_print(FILE *fp, int prec, int indent, const lspalge_t *alge);
int lspalge_prepare(lspalge_t *alge, lsenv_t *env, lserref_wrapper_t *erref);

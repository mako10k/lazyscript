#pragma once

#include <stdio.h>

typedef struct lsalge lsalge_t;

#include "array.h"
#include "eenv.h"
#include "erref.h"
#include "str.h"

typedef void (*lsalge_print_t)(FILE *stream, int prec, int indent,
                               const void *expr);
typedef int (*lsalge_prepare_t)(void *expr, lseenv_t *eenv,
                                lserref_wrapper_t *erref);

lsalge_t *lsalge(const lsstr_t *constr);
void lsalge_push_arg(lsalge_t *alge, void *arg);
void lsalge_push_args(lsalge_t *alge, const lsarray_t *args);
const lsstr_t *lsalge_get_constr(const lsalge_t *alge);
unsigned int lsalge_get_argc(const lsalge_t *alge);
void *lsalge_get_arg(const lsalge_t *alge, unsigned int i);
void lsalge_print(FILE *stream, int prec, int indent, const lsalge_t *alge,
                  lsalge_print_t lsprint);
int lsalge_prepare(lsalge_t *alge, lseenv_t *eenv, lserref_wrapper_t *erref,
                   lsalge_prepare_t lsprepare);
#pragma once

#include "array.h"
#include "str.h"

typedef struct lsalge lsalge_t;
typedef void (*lsalge_print_t)(FILE *, int, int, const void *);

lsalge_t *lsalge(const lsstr_t *constr);
void lsalge_push_arg(lsalge_t *alge, void *arg);
void lsalge_push_args(lsalge_t *alge, const lsarray_t *args);
const lsstr_t *lsalge_get_constr(const lsalge_t *alge);
unsigned int lsalge_get_argc(const lsalge_t *alge);
void *lsalge_get_arg(const lsalge_t *alge, unsigned int i);
void lsalge_print(FILE *fp, int prec, int indent, const lsalge_t *alge,
                  lsalge_print_t lsprint);
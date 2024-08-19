#pragma once

#include <stdlib.h>

typedef struct lsstr lsstr_t;

const lsstr_t *lsstr(const char *str, unsigned int len);
const lsstr_t *lsstr_cstr(const char *str);
const char *lsstr_get_buf(const lsstr_t *str);
unsigned int lsstr_get_len(const lsstr_t *str);
int lsstrcmp(const lsstr_t *str1, const lsstr_t *str2);
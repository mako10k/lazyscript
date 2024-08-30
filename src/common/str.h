#pragma once

#include <stdio.h>
#include <stdlib.h>

typedef struct lsstr lsstr_t;

#include "lstypes.h"

const lsstr_t *lsstr_new(const char *str, lssize_t len);
const lsstr_t *lsstr_sub(const lsstr_t *str, lssize_t pos, lssize_t len);
const lsstr_t *lsstr_cstr(const char *str);
const lsstr_t *lsstr_parse(const char *str, lssize_t len);
const char *lsstr_get_buf(const lsstr_t *str);
lssize_t lsstr_get_len(const lsstr_t *str);
int lsstrcmp(const lsstr_t *str1, const lsstr_t *str2);

/**
 * Calculates the hash value of a key.
 *
 * @param str The key to calculate the hash value.
 * @return The hash value of the key.
 */
unsigned int lsstr_calc_hash(const lsstr_t *str);

void lsstr_print(FILE *fp, lsprec_t prec, int indent, const lsstr_t *str);
void lsstr_print_bare(FILE *fp, lsprec_t prec, int indent, const lsstr_t *str);
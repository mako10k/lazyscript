#pragma once

#include <stdio.h>
#include <stdlib.h>

typedef struct lsstr lsstr_t;

const lsstr_t *lsstr(const char *str, unsigned int len);
const lsstr_t *lsstr_cstr(const char *str);
const lsstr_t *lsstr_parse(const char *str, unsigned int len);
const char *lsstr_get_buf(const lsstr_t *str);
unsigned int lsstr_get_len(const lsstr_t *str);
int lsstrcmp(const lsstr_t *str1, const lsstr_t *str2);
/**
 * Calculates the hash value of a key.
 *
 * @param key The key to calculate the hash value.
 * @return The hash value of the key.
 */
unsigned int lsstr_calc_hash(const lsstr_t *key);

void lsstr_print(FILE *fp, const lsstr_t *str);
void lsstr_print_bare(FILE *fp, const lsstr_t *str);
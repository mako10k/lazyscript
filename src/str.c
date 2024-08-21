#include "str.h"
#include "lazyscript.h"
#include "malloc.h"
#include <assert.h>
#include <gc.h>
#include <string.h>

struct lsstr {
  const char *buf;
  unsigned int len;
  const char _buf[0];
};

typedef struct lsstr_ht {
  const lsstr_t **ents;
  unsigned int size;
  unsigned int cap;
} lsstr_ht_t;

static lsstr_ht_t g_str_ht = {NULL, 0, 0};

/**
 * Calculates the hash value of a key.
 *
 * @param key The key to calculate the hash value.
 * @return The hash value of the key.
 */
__attribute__((nonnull(1), warn_unused_result)) static unsigned int
lsstr_calc_hash_bare(const char *buf, unsigned int len) {
  unsigned int hash = 0;
  for (unsigned int i = 0; i < len; i++)
    hash = hash * 31 + (buf[i] & 255);
  return hash;
}

/**
 * Calculates the hash value of a key.
 *
 * @param key The key to calculate the hash value.
 * @return The hash value of the key.
 */
unsigned int lsstr_calc_hash(const lsstr_t *str) {
  assert(str != NULL);
  return lsstr_calc_hash_bare(str->buf, str->len);
}

__attribute__((returns_nonnull, nonnull(1),
               warn_unused_result)) static const lsstr_t *
lsstr_raw(const char *buf, unsigned int len) {
  lsstr_t *str = lsmalloc_atomic(sizeof(lsstr_t) + len + 1);
  str->buf = str->_buf;
  str->len = len;
  memcpy((void *)str->buf, buf, len);
  ((char *)str->buf)[len] = '\0';
  return str;
}

__attribute__((returns_nonnull, nonnull(1, 2),
               warn_unused_result)) static const lsstr_t **
lsstr_ht_get_raw(lsstr_ht_t *str_ht, const char *buf, unsigned int len) {
  unsigned int hash = lsstr_calc_hash_bare(buf, len);
  unsigned int i = hash % str_ht->cap;
  while (1) {
    if (str_ht->ents[i] == NULL)
      break;
    if (str_ht->ents[i]->len == len &&
        memcmp(str_ht->ents[i]->buf, buf, len) == 0)
      break;
    i++;
    if (i >= str_ht->cap)
      i = 0;
  }
  return &str_ht->ents[i];
}

__attribute__((nonnull(1))) static void
lsstr_ht_resize(lsstr_ht_t *str_ht, unsigned int new_capacity) {
  assert(new_capacity >= str_ht->size);
  if (str_ht->cap == new_capacity)
    return;
  lsstr_ht_t new_str_hash = {NULL, 0, new_capacity};
  new_str_hash.ents = lsmalloc_atomic(sizeof(const lsstr_t *) * new_capacity);
  for (unsigned int i = 0; i < new_capacity; i++)
    new_str_hash.ents[i] = NULL;
  for (unsigned int i = 0; i < str_ht->cap; i++) {
    const lsstr_t *str = str_ht->ents[i];
    if (str != NULL) {
      const lsstr_t **pstr =
          lsstr_ht_get_raw(&new_str_hash, str->buf, str->len);
      assert(*pstr == NULL);
      *pstr = str;
    }
    new_str_hash.size = str_ht->size;
  }
  lsfree(str_ht->ents);
  str_ht->ents = new_str_hash.ents;
  str_ht->size = new_str_hash.size;
  str_ht->cap = new_str_hash.cap;
}

__attribute__((returns_nonnull, nonnull(1, 2),
               warn_unused_result)) static const lsstr_t **
lsstr_ht_put_raw(lsstr_ht_t *str_ht, const char *buf, unsigned int len) {
  const lsstr_t **pstr = lsstr_ht_get_raw(str_ht, buf, len);
  *pstr = lsstr_raw(buf, len);
  str_ht->size++;
  return pstr;
}

__attribute__((nonnull(1, 2))) static const lsstr_t *
lsstr_ht_del_raw(lsstr_ht_t *str_ht, const char *buf, unsigned int len) {
  const lsstr_t **pstr = lsstr_ht_get_raw(str_ht, buf, len);
  if (*pstr == NULL)
    return NULL;
  const lsstr_t *str = *pstr;
  unsigned int i = pstr - str_ht->ents;
  *pstr = NULL;
  while (1) {
    if (++i >= str_ht->cap)
      i = str_ht->cap;
    if (str_ht->ents[i] == NULL)
      break;
    const lsstr_t *str2 = str_ht->ents[i];
    str_ht->ents[i] = NULL;
    const lsstr_t **pstr2 = lsstr_ht_get_raw(str_ht, str2->buf, str2->len);
    assert(*pstr2 == NULL);
    *pstr2 = str2;
    pstr2++;
  }
  str_ht->size--;
  return str;
}

__attribute__((nonnull(1, 2), warn_unused_result,
               unused)) static const lsstr_t *
lsstr_hash_get(lsstr_ht_t *str_hash, const char *buf, unsigned int len) {
  assert(str_hash != NULL);
  return *lsstr_ht_get_raw(str_hash, buf, len);
}

__attribute__((nonnull(1, 2))) static const lsstr_t *
lsstr_ht_put(lsstr_ht_t *str_hash, const char *buf, unsigned int len) {
  unsigned int cap = str_hash->cap;
  while ((str_hash->size + 1) * 2 >= cap)
    cap *= 2;
  if (cap != str_hash->cap)
    lsstr_ht_resize(str_hash, cap);
  return *lsstr_ht_put_raw(str_hash, buf, len);
}

__attribute__((nonnull(1, 2))) static const lsstr_t *
lsstr_ht_del(lsstr_ht_t *str_hash, const lsstr_t *str) {
  const lsstr_t *str2 = lsstr_ht_del_raw(str_hash, str->buf, str->len);
  if (str2 == NULL)
    return NULL;
  unsigned int cap = str_hash->cap;
  while (str_hash->size * 4 <= cap)
    cap /= 2;
  if (cap != str_hash->cap)
    lsstr_ht_resize(str_hash, cap);
  return str;
}

__attribute__((nonnull(1))) static void lsstr_finalizer(void *ptr, void *data) {
  (void)data;
  lsstr_t *str = ptr;
  const lsstr_t *str2 = lsstr_ht_del(&g_str_ht, str);
  assert(str == str2);
}

__attribute__((constructor)) static void lsstr_init(void) {
  g_str_ht.ents = lsmalloc_atomic(sizeof(const lsstr_t *) * 16);
  g_str_ht.cap = 16;
  g_str_ht.size = 0;
  for (unsigned int i = 0; i < g_str_ht.cap; i++)
    g_str_ht.ents[i] = NULL;
}

const lsstr_t *lsstr(const char *buf, unsigned int len) {
  assert(buf != NULL);
  const lsstr_t *str = lsstr_ht_put(&g_str_ht, buf, len);
  GC_REGISTER_FINALIZER((void *)str, lsstr_finalizer, NULL, NULL, NULL);
  return str;
}

const lsstr_t *lsstr_cstr(const char *str) {
  assert(str != NULL);
  return lsstr(str, strlen(str));
}

const char *lsstr_get_buf(const lsstr_t *str) {
  assert(str != NULL);
  return str->buf;
}

unsigned int lsstr_get_len(const lsstr_t *str) {
  assert(str != NULL);
  return str->len;
}

int lsstrcmp(const lsstr_t *str1, const lsstr_t *str2) {
  assert(str1 != NULL);
  assert(str2 != NULL);
  if (str1 == str2)
    return 0;
  if (str1->len == str2->len)
    return memcmp(str1->buf, str2->buf, str1->len);
  else if (str1->len < str2->len) {
    int cmp = memcmp(str1->buf, str2->buf, str1->len);
    return cmp == 0 ? -1 : cmp;
  } else {
    int cmp = memcmp(str1->buf, str2->buf, str2->len);
    return cmp == 0 ? 1 : cmp;
  }
}

__attribute__((nonnull(1, 2), warn_unused_result)) static int
lsstr_parse_hex(const char *str, unsigned int *pi, unsigned int slen) {
  int hex = 0;
  int c = *pi < slen ? str[(*pi)++] : -1;
  if (c == -1)
    return -1;

  if ('0' <= c && c <= '9')
    hex = c - '0';
  else if ('a' <= c && c <= 'f')
    hex = c - 'a' + 10;
  else if ('A' <= c && c <= 'F')
    hex = c - 'A' + 10;
  else
    return 'x';
  c = *pi < slen ? str[(*pi)++] : -1;
  if (c == -1)
    return -1;
  if ('0' <= c && c <= '9')
    hex = hex * 16 + (c - '0');
  else if ('a' <= c && c <= 'f')
    hex = hex * 16 + (c - 'a' + 10);
  else if ('A' <= c && c <= 'F')
    hex = hex * 16 + (c - 'A' + 10);
  else
    return hex;
  return hex;
}

__attribute__((nonnull(1, 2), warn_unused_result)) static int
lsstr_parse_oct(const char *str, unsigned int *pi, unsigned int slen, int oct) {
  oct = oct - '0';
  int c = *pi < slen ? str[(*pi)++] : -1;
  if (c == -1)
    return -1;
  if ('0' <= c && c <= '7')
    oct = oct * 8 + (c - '0');
  else
    return oct;
  return oct;
}

__attribute__((nonnull(1, 2), warn_unused_result)) static int
lsstr_parse_esc(const char *str, unsigned int *pi, unsigned int slen) {
  int c = *pi < slen ? str[(*pi)++] : -1;
  switch (c) {
  case -1:
    return -1;
  case 'n':
  case 'N':
    c = '\n';
    break;
  case 't':
  case 'T':
    c = '\t';
    break;
  case 'r':
  case 'R':
    c = '\r';
    break;
  case '0':
    c = '\0';
    break;
  case '\\':
  case '"':
  case '\'':
    break;
  case 'a':
  case 'A':
    c = '\a';
    break;
  case 'b':
  case 'B':
    c = '\b';
    break;
  case 'f':
  case 'F':
    c = '\f';
    break;
  case 'v':
  case 'V':
    c = '\v';
    break;
  case 'x':
  case 'X':
    c = lsstr_parse_hex(str, pi, slen);
    break;
  default:
    if ('0' <= c && c <= '7')
      c = lsstr_parse_oct(str, pi, slen, c);
  }
  return c;
}

const lsstr_t *lsstr_parse(const char *cstr, unsigned int slen) {
  assert(cstr != NULL);
  unsigned int i = 0;
  int c;
  c = i < slen ? cstr[i++] : -1;
  if (c != '"')
    return NULL;
  char *strval = lsmalloc(16);
  size_t len = 0;
  size_t cap = 16;
  while (1) {
    c = i < len ? cstr[i++] : -1;
    if (c == '"')
      break;
    if (c == '\\')
      c = lsstr_parse_esc(cstr, &i, slen);
    if (c == -1)
      return NULL;
    if (cap <= len + 1) {
      cap *= 2;
      strval = lsrealloc(strval, cap);
    }
    strval[len++] = c;
  }
  strval[len] = '\0';
  strval = lsrealloc(strval, len + 1);
  return lsstr(strval, len);
}

void lsstr_print(FILE *fp, int prec, int indent, const lsstr_t *str) {
  assert(fp != NULL);
  assert(str != NULL);
  (void)prec;
  lsprintf(fp, indent, "\"");
  const char *cstr = str->buf;
  unsigned int len = str->len;
  for (unsigned int i = 0; i < len; i++) {
    switch (cstr[i]) {
    case '\n':
      lsprintf(fp, indent, "\\n");
      break;
    case '\t':
      lsprintf(fp, indent, "\\t");
      break;
    case '\r':
      lsprintf(fp, indent, "\\r");
      break;
    case '\0':
      lsprintf(fp, indent, "\\0");
      break;
    case '\\':
      lsprintf(fp, indent, "\\\\");
      break;
    case '"':
      lsprintf(fp, indent, "\\\"");
      break;
    case '\'':
      lsprintf(fp, indent, "\\'");
      break;
    case '\a':
      lsprintf(fp, indent, "\\a");
      break;
    case '\b':
      lsprintf(fp, indent, "\\b");
      break;
    case '\f':
      lsprintf(fp, indent, "\\f");
      break;
    case '\v':
      lsprintf(fp, indent, "\\v");
      break;
    default:
      if (cstr[i] < 32 || cstr[i] >= 127)
        lsprintf(fp, indent, "\\x%02x", cstr[i]);
      else
        lsprintf(fp, indent, "%c", cstr[i]);
    }
  }
  lsprintf(fp, 0, "\"");
}

void lsstr_print_bare(FILE *fp, int prec, int indent, const lsstr_t *str) {
  assert(fp != NULL);
  assert(str != NULL);
  (void)prec;
  (void)indent;
  fwrite(str->buf, 1, str->len, fp);
}
#include "common/str.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>
#include <gc.h>
#include <limits.h>
#include <string.h>

struct lsstr {
  const char *lst_buf;
  lssize_t lst_len;
};

typedef struct lsstr_ht {
  const lsstr_t **lsth_ents;
  lssize_t lsth_size;
  lssize_t lsth_cap;
} lsstr_ht_t;

static lsstr_ht_t g_str_ht = {NULL, 0, 0};

/**
 * Calculates the hash value of a key.
 *
 * @param key The key to calculate the hash value.
 * @return The hash value of the key.
 */
static unsigned int lsstr_calc_hash_bare(const char *const buf,
                                         const lssize_t len) {
  assert(buf != NULL);
  unsigned int hash = 0;
  for (lssize_t i = 0; i < len; i++)
    hash = hash * 31 + (buf[i] & 255);
  return hash;
}

/**
 * Calculates the hash value of a key.
 *
 * @param key The key to calculate the hash value.
 * @return The hash value of the key.
 */
unsigned int lsstr_calc_hash(const lsstr_t *const str) {
  assert(str != NULL);
  return lsstr_calc_hash_bare(str->lst_buf, str->lst_len);
}

static const lsstr_t *lsstr_raw(const char *const buf, const lssize_t len) {
  assert(buf != NULL);
  lsstr_t *const str = lsmalloc(sizeof(lsstr_t) + len + 1);
  str->lst_buf = lsmalloc_atomic(len + 1);
  str->lst_len = len;
  memcpy((void *)str->lst_buf, buf, len);
  ((char *)str->lst_buf)[len] = '\0';
  return str;
}

static const lsstr_t **lsstr_ht_get_raw(lsstr_ht_t *const str_ht,
                                        const char *const buf,
                                        const lssize_t len) {
  assert(str_ht != NULL);
  assert(buf != NULL);
  const unsigned int hash = lsstr_calc_hash_bare(buf, len);
  const lssize_t cap = str_ht->lsth_cap;
  const lsstr_t **const ents = str_ht->lsth_ents;
  lssize_t i = hash % cap;
  while (1) {
    const lsstr_t *const ent = ents[i];
    if (ent == NULL)
      return &ents[i];
    if (lsstrcmp(ent, &(lsstr_t){.lst_buf = buf, .lst_len = len}) == 0)
      return &ents[i];
    i++;
    if (i >= cap)
      i = 0;
  }
}

static void lsstr_ht_resize(lsstr_ht_t *const str_ht,
                            const lssize_t new_capacity) {
  assert(str_ht != NULL);
  assert(new_capacity >= str_ht->lsth_size);
  if (str_ht->lsth_cap == new_capacity)
    return;
  lsstr_ht_t new_str_hash = {NULL, 0, new_capacity};
  new_str_hash.lsth_ents =
      lsmalloc_atomic(sizeof(const lsstr_t *) * new_capacity);
  for (lssize_t i = 0; i < new_capacity; i++)
    new_str_hash.lsth_ents[i] = NULL;
  for (lssize_t i = 0; i < str_ht->lsth_cap; i++) {
    const lsstr_t *const ent = str_ht->lsth_ents[i];
    if (ent != NULL) {
      const char *const ebuf = ent->lst_buf;
      const lssize_t elen = ent->lst_len;
      const lsstr_t **const pstr = lsstr_ht_get_raw(&new_str_hash, ebuf, elen);
      assert(*pstr == NULL);
      *pstr = ent;
    }
    new_str_hash.lsth_size = str_ht->lsth_size;
  }
  lsfree(str_ht->lsth_ents);
  str_ht->lsth_ents = new_str_hash.lsth_ents;
  str_ht->lsth_size = new_str_hash.lsth_size;
  str_ht->lsth_cap = new_str_hash.lsth_cap;
}

static const lsstr_t **lsstr_ht_put_raw(lsstr_ht_t *const str_ht,
                                        const char *const buf,
                                        const lssize_t len) {
  assert(str_ht != NULL);
  assert(buf != NULL);
  const lsstr_t **const pstr = lsstr_ht_get_raw(str_ht, buf, len);
  *pstr = lsstr_raw(buf, len);
  str_ht->lsth_size++;
  return pstr;
}

static const lsstr_t *lsstr_ht_del_raw(lsstr_ht_t *const str_ht,
                                       const char *const buf,
                                       const lssize_t len) {
  assert(str_ht != NULL);
  assert(buf != NULL);
  const lsstr_t **const pstr = lsstr_ht_get_raw(str_ht, buf, len);
  if (*pstr == NULL)
    return NULL;
  const lsstr_t *const str = *pstr;
  const lsstr_t **const ents = str_ht->lsth_ents;
  lssize_t i = pstr - ents;
  *pstr = NULL;
  while (1) {
    if (++i >= str_ht->lsth_cap)
      i = str_ht->lsth_cap;
    const lsstr_t *const ent = ents[i];
    if (ent == NULL)
      break;
    ents[i] = NULL;
    {
      const lsstr_t **const pstr2 =
          lsstr_ht_get_raw(str_ht, ent->lst_buf, ent->lst_len);
      assert(*pstr2 == NULL);
      *pstr2 = ent;
    }
  }
  str_ht->lsth_size--;
  return str;
}

static const lsstr_t *lsstr_ht_put(lsstr_ht_t *const str_ht,
                                   const char *const buf, const lssize_t len) {
  assert(str_ht != NULL);
  assert(buf != NULL);
  lssize_t cap = str_ht->lsth_cap;
  while ((str_ht->lsth_size + 1) * 2 >= cap)
    cap *= 2;
  if (cap != str_ht->lsth_cap)
    lsstr_ht_resize(str_ht, cap);
  return *lsstr_ht_put_raw(str_ht, buf, len);
}

static const lsstr_t *lsstr_ht_del(lsstr_ht_t *const str_hash,
                                   const lsstr_t *const str) {
  assert(str_hash != NULL);
  assert(str != NULL);
  const lsstr_t *const str2 =
      lsstr_ht_del_raw(str_hash, str->lst_buf, str->lst_len);
  if (str2 == NULL)
    return NULL;
  lssize_t cap = str_hash->lsth_cap;
  while (str_hash->lsth_size * 4 <= cap)
    cap /= 2;
  if (cap != str_hash->lsth_cap)
    lsstr_ht_resize(str_hash, cap);
  return str;
}

static void lsstr_finalizer(void *const ptr, void *const data) {
  assert(ptr != NULL);
  (void)data;
  const lsstr_t *str = ptr;
  const lsstr_t *const str2 = lsstr_ht_del(&g_str_ht, str);
  assert(str == str2);
}

__attribute__((constructor)) static void lsstr_init(void) {
  g_str_ht.lsth_ents = lsmalloc_atomic(sizeof(const lsstr_t *) * 16);
  g_str_ht.lsth_cap = 16;
  g_str_ht.lsth_size = 0;
  for (lssize_t i = 0; i < g_str_ht.lsth_cap; i++)
    g_str_ht.lsth_ents[i] = NULL;
}

const lsstr_t *lsstr(const char *const buf, const lssize_t len) {
  assert(buf != NULL);
  const lsstr_t *const str = lsstr_ht_put(&g_str_ht, buf, len);
  GC_REGISTER_FINALIZER((void *)str, lsstr_finalizer, NULL, NULL, NULL);
  return str;
}

const lsstr_t *lsstr_sub(const lsstr_t *const str, const lssize_t pos,
                         const lssize_t len) {
  assert(str != NULL);
  assert(pos <= str->lst_len);
  assert(len <= str->lst_len - pos);
  if (pos == 0 && len == str->lst_len)
    return str;
  lsstr_t *const sub = lsmalloc(sizeof(lsstr_t));
  sub->lst_buf = str->lst_buf + pos;
  sub->lst_len = len;
  return sub;
}

const lsstr_t *lsstr_cstr(const char *const str) {
  assert(str != NULL);
  return lsstr(str, strlen(str));
}

const char *lsstr_get_buf(const lsstr_t *const str) {
  assert(str != NULL);
  return str->lst_buf;
}

lssize_t lsstr_get_len(const lsstr_t *const str) {
  assert(str != NULL);
  return str->lst_len;
}

int lsstrcmp(const lsstr_t *const str1, const lsstr_t *const str2) {
  assert(str1 != NULL);
  assert(str2 != NULL);
  if (str1 == str2)
    return 0;
  const char *const buf1 = str1->lst_buf;
  const char *const buf2 = str2->lst_buf;
  if (buf1 == buf2)
    return 0;
  const lssize_t len1 = str1->lst_len;
  const lssize_t len2 = str2->lst_len;
  const lssize_t len = len1 < len2 ? len1 : len2;
  const int cmp = memcmp(str1->lst_buf, str2->lst_buf, len);
  if (cmp != 0)
    return cmp;
  return len1 - len2;
}

__attribute__((nonnull(1, 2), warn_unused_result)) static int
lsstr_parse_hex(const char *const str, lssize_t *const pi,
                const lssize_t slen) {
  int hex = 0;
  const int c1 = *pi < slen ? str[(*pi)++] : -1;
  if (c1 == -1)
    return -1;

  if ('0' <= c1 && c1 <= '9')
    hex = c1 - '0';
  else if ('a' <= c1 && c1 <= 'f')
    hex = c1 - 'a' + 10;
  else if ('A' <= c1 && c1 <= 'F')
    hex = c1 - 'A' + 10;
  else
    return 'x';

  const int c2 = *pi < slen ? str[(*pi)++] : -1;
  if (c2 == -1)
    return -1;
  if ('0' <= c2 && c2 <= '9')
    hex = hex * 16 + (c2 - '0');
  else if ('a' <= c2 && c2 <= 'f')
    hex = hex * 16 + (c2 - 'a' + 10);
  else if ('A' <= c2 && c2 <= 'F')
    hex = hex * 16 + (c2 - 'A' + 10);
  return hex;
}

__attribute__((nonnull(1, 2), warn_unused_result)) static int
lsstr_parse_oct(const char *const str, lssize_t *const pi, const lssize_t slen,
                int oct) {
  oct = oct - '0';
  const int c = *pi < slen ? str[(*pi)++] : -1;
  if (c == -1)
    return -1;
  if ('0' <= c && c <= '7')
    oct = oct * 8 + (c - '0');
  else
    return oct;
  return oct;
}

__attribute__((nonnull(1, 2), warn_unused_result)) static int
lsstr_parse_esc(const char *const str, lssize_t *const pi,
                const lssize_t slen) {
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

const lsstr_t *lsstr_parse(const char *const cstr, const lssize_t slen) {
  assert(cstr != NULL);
  lssize_t i = 0;
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

void lsstr_print(FILE *const fp, const lsprec_t prec, const int indent,
                 const lsstr_t *const str) {
  assert(fp != NULL);
  assert(str != NULL);
  (void)prec;
  lsprintf(fp, indent, "\"");
  const char *const cstr = str->lst_buf;
  const lssize_t len = str->lst_len;
  for (lssize_t i = 0; i < len; i++) {
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

void lsstr_print_bare(FILE *const fp, const lsprec_t prec, const int indent,
                      const lsstr_t *const str) {
  assert(fp != NULL);
  assert(str != NULL);
  (void)prec;
  (void)indent;
  fwrite(str->lst_buf, 1, str->lst_len, fp);
}
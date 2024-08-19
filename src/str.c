#include "str.h"
#include "malloc.h"
#include <string.h>

struct lsstr {
  const char *buf;
  unsigned int len;
};

const lsstr_t *lsstr(const char *buf, unsigned int len) {
  lsstr_t *s = lsmalloc(sizeof(lsstr_t));
  s->buf = buf;
  s->len = len;
  return s;
}

const lsstr_t *lsstr_cstr(const char *str) { return lsstr(str, strlen(str)); }

const char *lsstr_get_buf(const lsstr_t *str) { return str->buf; }

unsigned int lsstr_get_len(const lsstr_t *str) { return str->len; }

int lsstrcmp(const lsstr_t *str1, const lsstr_t *str2) {
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
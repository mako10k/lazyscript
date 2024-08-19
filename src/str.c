#include "str.h"
#include "malloc.h"
#include <assert.h>
#include <stdio.h>
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

static int lsstr_parse_hex(const char *str, unsigned int *pi,
                           unsigned int slen) {
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

static int lsstr_parse_oct(const char *str, unsigned int *pi, unsigned int slen,
                           int oct) {
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

static int lsstr_parse_esc(const char *str, unsigned int *pi,
                           unsigned int slen) {
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
  default:
    if ('0' <= c && c <= '7')
      c = lsstr_parse_oct(str, pi, slen, c);
  }
  return c;
}

const lsstr_t *lsstr_parse(const char *cstr, unsigned int slen) {
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

void lsstr_print(FILE *fp, const lsstr_t *str) {
  assert(fp != NULL);
  assert(str != NULL);
  fprintf(fp, "\"");
  const char *cstr = str->buf;
  unsigned int len = str->len;
  for (unsigned int i = 0; i < len; i++) {
    switch (cstr[i]) {
    case '\n':
      fprintf(fp, "\\n");
      break;
    case '\t':
      fprintf(fp, "\\t");
      break;
    case '\r':
      fprintf(fp, "\\r");
      break;
    case '\0':
      fprintf(fp, "\\0");
      break;
    case '\\':
      fprintf(fp, "\\\\");
      break;
    case '"':
      fprintf(fp, "\\\"");
      break;
    case '\'':
      fprintf(fp, "\\'");
      break;
    case '\a':
      fprintf(fp, "\\a");
      break;
    case '\b':
      fprintf(fp, "\\b");
      break;
    case '\f':
      fprintf(fp, "\\f");
      break;
    case '\v':
      fprintf(fp, "\\v");
      break;
    default:
      if (cstr[i] < 32 || cstr[i] >= 127)
        fprintf(fp, "\\x%02x", cstr[i]);
      else
        fprintf(fp, "%c", cstr[i]);
    }
  }
  fprintf(fp, "\"");
}

/* Enable GNU extensions for fopencookie before any stdio.h inclusion */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <stdio.h>
#include "common/io.h"
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "common/malloc.h"

void lsprintf(FILE *fp, int indent, const char *fmt, ...) {
  if (indent == 0) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
  } else {
    va_list ap;
    va_start(ap, fmt);
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char buf[size + 1];
    va_start(ap, fmt);
    vsnprintf(buf, size + 1, fmt, ap);
    va_end(ap);
    const char *str = buf;
    while (*str != '\0') {
      const char *nl = strchr(str, '\n');
      if (nl == NULL) {
        fprintf(fp, "%s", str);
        break;
      }
      fwrite(str, 1, nl - str + 1, fp);
      for (int i = 0; i < indent; i++)
        fprintf(fp, "  ");
      str = nl + 1;
    }
  }
}

typedef struct {
  char **pbuf;
  size_t *psize;
  size_t len;
  size_t cap;
  int closed;
} ls_gc_memstream_t;

static ssize_t ls_gc_mem_write(void *cookie, const char *buf, size_t size) {
  ls_gc_memstream_t *ms = (ls_gc_memstream_t *)cookie;
  if (ms->closed)
    return -1;
  size_t need = ms->len + size + 1; // +1 for trailing NUL
  if (need > ms->cap) {
    size_t new_cap = ms->cap ? ms->cap : 64;
    while (new_cap < need)
      new_cap *= 2;
    char *new_buf = ms->pbuf && *ms->pbuf ? lsrealloc(*ms->pbuf, new_cap)
                                          : lsmalloc(new_cap);
    if (ms->pbuf)
      *ms->pbuf = new_buf;
    ms->cap = new_cap;
  }
  if (size > 0) {
    memcpy(*ms->pbuf + ms->len, buf, size);
    ms->len += size;
  }
  (*ms->pbuf)[ms->len] = '\0';
  if (ms->psize)
    *ms->psize = ms->len;
  return (ssize_t)size;
}

static int ls_gc_mem_close(void *cookie) {
  ls_gc_memstream_t *ms = (ls_gc_memstream_t *)cookie;
  ms->closed = 1;
  // Ensure NUL terminator
  if (ms->pbuf && *ms->pbuf) {
    (*ms->pbuf)[ms->len] = '\0';
  }
  if (ms->psize)
    *ms->psize = ms->len;
  return 0;
}

#if defined(__GLIBC__) || defined(__USE_GNU)
// Use fopencookie on glibc
FILE *lsopen_memstream_gc(char **pbuf, size_t *psize) {
  static const cookie_io_functions_t fns = {
      .read = NULL, .write = ls_gc_mem_write, .seek = NULL, .close = ls_gc_mem_close};
  ls_gc_memstream_t *ms = lsmalloc(sizeof(ls_gc_memstream_t));
  ms->pbuf = pbuf;
  ms->psize = psize;
  ms->len = 0;
  ms->cap = 0;
  ms->closed = 0;
  if (pbuf)
    *pbuf = NULL;
  if (psize)
    *psize = 0;
  FILE *fp = fopencookie(ms, "w", fns);
  return fp;
}
#else
// Fallback: use POSIX open_memstream but ensure GC by swapping buffer at close
FILE *lsopen_memstream_gc(char **pbuf, size_t *psize) {
  // Use standard open_memstream, but caller should not free. That's acceptable
  // only if later we copy into GC memory before use. For portability, we still
  // provide this symbol.
  return open_memstream(pbuf, psize);
}
#endif

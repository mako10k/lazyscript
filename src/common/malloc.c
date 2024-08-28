#include "common/malloc.h"
#include <gc.h>

void *lsmalloc(size_t size) { return GC_MALLOC(size); }

void *lsmalloc_atomic(size_t size) { return GC_MALLOC_ATOMIC(size); }

void *lsrealloc(void *ptr, size_t size) { return GC_REALLOC(ptr, size); }

void lsfree(void *ptr) { GC_FREE(ptr); }
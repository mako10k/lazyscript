#include "malloc.h"
#include <gc.h>

void *lsmalloc(size_t size) { return GC_MALLOC(size); }

void *lsrealloc(void *ptr, size_t size) { return GC_REALLOC(ptr, size); }

void lsfree(void *ptr) { GC_FREE(ptr); }
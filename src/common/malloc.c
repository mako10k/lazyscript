#include "common/malloc.h"
#include <gc.h>
#include <stdlib.h>

static int use_libc_alloc(void) {
	static int cached = -1;
	if (cached >= 0) return cached;
	const char* v = getenv("LAZYSCRIPT_USE_LIBC_ALLOC");
	cached = (v && v[0] && v[0] != '0') ? 1 : 0;
	return cached;
}

void* lsmalloc(size_t size) {
	if (use_libc_alloc()) return malloc(size);
	return GC_MALLOC(size);
}

void* lsmalloc_atomic(size_t size) {
	if (use_libc_alloc()) return malloc(size);
	return GC_MALLOC_ATOMIC(size);
}

void* lsrealloc(void* ptr, size_t size) {
	if (use_libc_alloc()) return realloc(ptr, size);
	return GC_REALLOC(ptr, size);
}

void  lsfree(void* ptr) {
	if (use_libc_alloc()) { free(ptr); return; }
	GC_FREE(ptr);
}
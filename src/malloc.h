#pragma once

#include <stdlib.h>

void *lsmalloc(size_t size);
void *lsmalloc_atomic(size_t size);
void *lsrealloc(void *ptr, size_t size);
void lsfree(void *ptr);
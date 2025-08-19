#pragma once

#include "common/loc.h"
/* Enable GNU extensions (for fopencookie) before including stdio.h */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <stdio.h>

/** Print a string.
 * @param fp File pointer.
 * @param indent Indentation.
 * @param fmt Format String.
 */
void lsprintf(FILE* fp, int indent, const char* fmt, ...);

/* GC 管理のメモリストリームを返す。close/fflush 後に *pbuf と *psize が更新され、
 * バッファは GC により管理されるため free は不要。 */
FILE* lsopen_memstream_gc(char** pbuf, size_t* psize);
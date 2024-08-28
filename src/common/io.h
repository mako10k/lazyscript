#pragma once

#include "common/loc.h"
#include <stdio.h>

/** Print a string.
 * @param fp File pointer.
 * @param indent Indentation.
 * @param fmt Format String.
 */
void lsprintf(FILE *fp, int indent, const char *fmt, ...);
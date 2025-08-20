#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_VERSION "0.0.1"
#define PACKAGE_NAME "lazyscript"
#endif

#include <stdio.h>
#include "misc/prog.h"

/* パーサAPI: lazyscript_format から利用 */
extern const lsprog_t* lsparse_stream(const char* filename, FILE* in_str);

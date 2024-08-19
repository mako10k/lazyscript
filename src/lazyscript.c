#include "malloc.h"
#include <stdarg.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "lazyscript.h"
#include "parser.h"
#include <gc.h>

int main() { yyparse(); }


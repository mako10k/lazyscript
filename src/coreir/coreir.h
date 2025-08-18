#pragma once

typedef struct lscir lscir_t;
typedef struct lscir_prog lscir_prog_t;

#include <stdio.h>
#include "misc/prog.h"

/**
 * Lower a parsed program to Core IR.
 * For now this is a thin wrapper around the AST to establish the module boundary.
 */
const lscir_prog_t *lscir_lower_prog(const lsprog_t *prog);

/**
 * Print a Core IR program.
 */
void lscir_print(FILE *fp, int indent, const lscir_prog_t *cir);

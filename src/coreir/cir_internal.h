#pragma once

#include "coreir/coreir.h"
#include "misc/prog.h"

// Internal representation of a Core IR program root
struct lscir_prog {
  const lsprog_t *ast;      // original AST (may be NULL)
  const lscir_expr_t *root; // lowered Core IR root (may be NULL)
};

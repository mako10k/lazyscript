#include "prog.h"

struct lsprog {
  lsexpr_t *expr;
};

lsprog_t *lsprog(lsexpr_t *expr) {
  lsprog_t *prog = malloc(sizeof(lsprog_t));
  prog->expr = expr;
  return prog;
}
#include "lstr.h"
#include "lstr_internal.h"

static int validate_expr(const lstr_expr_t* e) {
  if (!e)
    return -1;
  switch (e->kind) {
  case LSTR_EXP_VAL:
    return e->as.v ? 0 : -2;
  case LSTR_EXP_APP:
    if (!e->as.app.func)
      return -3;
    for (size_t i = 0; i < e->as.app.argc; i++)
      if (!e->as.app.args[i])
        return -4;
    return 0;
  default:
    return 0;
  }
}

int lstr_validate(const lstr_prog_t* p) {
  if (!p)
    return -10;
  for (size_t i = 0; i < p->rootc; i++) {
    int rc = validate_expr(p->roots[i]);
    if (rc != 0)
      return rc;
  }
  return 0;
}

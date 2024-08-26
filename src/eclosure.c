#include "eclosure.h"
#include "expr.h"
#include "io.h"
#include "lazyscript.h"

struct lseclosure {
  lsexpr_t *expr;
  lsbind_t *bind;
};

lseclosure_t *lseclosure(lsexpr_t *expr, lsbind_t *bind) {
  lseclosure_t *eclosure = malloc(sizeof(lseclosure_t));
  eclosure->expr = expr;
  eclosure->bind = bind;
  return eclosure;
}

void lseclosure_print(FILE *stream, int prec, int indent,
                      lseclosure_t *eclosure) {
  unsigned int lines = lsbind_get_count(eclosure->bind) + 1;
  if (lsexpr_type(eclosure->expr) == LSETYPE_LAMBDA)
    lines += lselambda_get_count(lsexpr_get_lambda(eclosure->expr)) - 1;
  if (lines > 1) {
    indent++;
    lsprintf(stream, indent, "{\n");
  } else
    lsprintf(stream, indent, "{ ");
  lsexpr_print(stream, prec, indent, eclosure->expr);
  lsbind_print(stream, prec, indent, eclosure->bind);
  if (lines > 1) {
    indent--;
    lsprintf(stream, indent, "\n}");
  } else
    lsprintf(stream, indent, " }");
}

int lseclosure_prepare(lseclosure_t *eclosure, lseenv_t *eenv) {
  unsigned int nbinds = lsbind_get_count(eclosure->bind);
  eenv = lseenv(eenv);
  for (unsigned int i = 0; i < nbinds; i++) {
    lsbind_ent_t *bind_ent = lsbind_get_ent(eclosure->bind, i);
    lspat_t *pat = lsbind_ent_get_pat(bind_ent);
    int res = lspat_prepare(pat, eenv, lserref_wrapper_bind_ent(bind_ent));
    if (res < 0)
      return res;
  }
  for (unsigned int i = 0; i < nbinds; i++) {
    lsbind_ent_t *bind_ent = lsbind_get_ent(eclosure->bind, i);
    lsexpr_t *expr = lsbind_ent_get_expr(bind_ent);
    int res = lsexpr_prepare(expr, eenv);
    if (res < 0)
      return res;
  }
  return lsexpr_prepare(eclosure->expr, eenv);
}
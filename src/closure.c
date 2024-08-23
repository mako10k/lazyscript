#include "closure.h"
#include "expr.h"
#include "lazyscript.h"

struct lsclosure {
  lsexpr_t *expr;
  lsbind_t *bind;
};

lsclosure_t *lsclosure(lsexpr_t *expr, lsbind_t *bind) {
  lsclosure_t *closure = malloc(sizeof(lsclosure_t));
  closure->expr = expr;
  closure->bind = bind;
  return closure;
}

void lsclosure_print(FILE *fp, int prec, int indent, lsclosure_t *closure) {
  unsigned int lines = lsbind_get_count(closure->bind) + 1;
  if (lsexpr_type(closure->expr) == LSETYPE_LAMBDA)
    lines += lslambda_get_count(lsexpr_get_lambda(closure->expr)) - 1;
  if (lines > 1) {
    indent++;
    lsprintf(fp, indent, "{\n");
  } else
    lsprintf(fp, indent, "{ ");
  lsexpr_print(fp, prec, indent, closure->expr);
  lsbind_print(fp, prec, indent, closure->bind);
  if (lines > 1) {
    indent--;
    lsprintf(fp, indent, "\n}");
  } else
    lsprintf(fp, indent, " }");
}

int lsclosure_prepare(lsclosure_t *closure, lsenv_t *env) {
  unsigned int bcount = lsbind_get_count(closure->bind);
  env = lsenv(env);
  for (unsigned int i = 0; i < bcount; i++) {
    lsbind_ent_t *bent = lsbind_get_ent(closure->bind, i);
    lspat_t *pat = lsbind_ent_get_pat(bent);
    int ret = lspat_prepare(pat, env, lserref_bind_ent(bent));
    if (ret < 0)
      return ret;
  }
  for (unsigned int i = 0; i < bcount; i++) {
    lsbind_ent_t *bent = lsbind_get_ent(closure->bind, i);
    lsexpr_t *expr = lsbind_ent_get_expr(bent);
    int ret = lsexpr_prepare(expr, env);
    if (ret < 0)
      return ret;
  }
  return lsexpr_prepare(closure->expr, env);
}
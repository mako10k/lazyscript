#include "expr/echoice.h"
#include "common/io.h"
#include "common/malloc.h"
#include <assert.h>

struct lsechoice {
  lsexpr_t *lec_left;
  lsexpr_t *lec_right;
};

lsechoice_t *lsechoice_new(lsexpr_t *left, lsexpr_t *right) {
  assert(left != NULL);
  assert(right != NULL);
  lsechoice_t *echoice = lsmalloc(sizeof(lsechoice_t));
  echoice->lec_left = left;
  echoice->lec_right = right;
  return echoice;
}

lsexpr_t *lsechoice_get_left(const lsechoice_t *echoice) {
  assert(echoice != NULL);
  return echoice->lec_left;
}

lsexpr_t *lsechoice_get_right(const lsechoice_t *echoice) {
  assert(echoice != NULL);
  return echoice->lec_right;
}
void lsechoice_print(FILE *fp, lsprec_t prec, int indent,
                     const lsechoice_t *echoice) {
  assert(fp != NULL);
  assert(echoice != NULL);
  if (prec > LSPREC_CHOICE)
    lsprintf(fp, indent, "(");
  indent++;
  lsprintf(fp, indent, "\n");
  while (1) {
    lsexpr_print(fp, LSPREC_CHOICE + 1, indent, echoice->lec_left);
    lsprintf(fp, indent, " |\n");
    if (lsexpr_get_type(echoice->lec_right) != LSETYPE_CHOICE)
      break;
    echoice = lsexpr_get_choice(echoice->lec_right);
  }
  lsexpr_print(fp, LSPREC_CHOICE, indent, echoice->lec_right);
  indent--;
  if (prec > LSPREC_CHOICE)
    lsprintf(fp, indent, "\n)");
}

lspres_t lsechoice_prepare(lsechoice_t *echoice, lseenv_t *env) {
  lspres_t res = lsexpr_prepare(echoice->lec_left, env);
  if (res != LSPRES_SUCCESS)
    return res;
  return lsexpr_prepare(echoice->lec_right, env);
}
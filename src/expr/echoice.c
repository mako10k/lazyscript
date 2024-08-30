#include "expr/echoice.h"
#include "common/io.h"
#include "common/malloc.h"

struct lsechoice {
  const lsexpr_t *lec_left;
  const lsexpr_t *lec_right;
};

const lsechoice_t *lsechoice_new(const lsexpr_t *left, const lsexpr_t *right) {
  lsechoice_t *echoice = lsmalloc(sizeof(lsechoice_t));
  echoice->lec_left = left;
  echoice->lec_right = right;
  return echoice;
}

const lsexpr_t *lsechoice_get_left(const lsechoice_t *echoice) {
  return echoice->lec_left;
}

const lsexpr_t *lsechoice_get_right(const lsechoice_t *echoice) {
  return echoice->lec_right;
}

void lsechoice_print(FILE *fp, lsprec_t prec, int indent,
                     const lsechoice_t *echoice) {
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
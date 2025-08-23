#include "expr/echoice.h"
#include "common/io.h"
#include "common/malloc.h"

struct lsechoice {
  const lsexpr_t* lec_left;
  const lsexpr_t* lec_right;
};

const lsechoice_t* lsechoice_new(const lsexpr_t* left, const lsexpr_t* right) {
  lsechoice_t* echoice = lsmalloc(sizeof(lsechoice_t));
  echoice->lec_left    = left;
  echoice->lec_right   = right;
  return echoice;
}

const lsexpr_t* lsechoice_get_left(const lsechoice_t* echoice) { return echoice->lec_left; }

const lsexpr_t* lsechoice_get_right(const lsechoice_t* echoice) { return echoice->lec_right; }

// Helpers to inspect nested choice chains
static const lsexpr_t* choice_leftmost_term(const lsechoice_t* ch) {
  // Follow left spine until non-choice
  while (lsexpr_get_type(ch->lec_left) == LSETYPE_CHOICE)
    ch = lsexpr_get_choice(ch->lec_left);
  return ch->lec_left;
}

static const lsexpr_t* choice_rightmost_term(const lsechoice_t* ch) {
  // Follow right spine until non-choice
  while (lsexpr_get_type(ch->lec_right) == LSETYPE_CHOICE)
    ch = lsexpr_get_choice(ch->lec_right);
  return ch->lec_right;
}

void            lsechoice_print(FILE* fp, lsprec_t prec, int indent, const lsechoice_t* echoice) {
             if (prec > LSPREC_CHOICE)
    lsprintf(fp, indent, "(");
  indent++;
             lsprintf(fp, indent, "\n");
             while (1) {
               // Print the left side; avoid wrapping nested choices in parens by delegating with same precedence
               if (lsexpr_get_type(echoice->lec_left) == LSETYPE_CHOICE)
      lsechoice_print(fp, LSPREC_CHOICE, indent, lsexpr_get_choice(echoice->lec_left));
               else
      lsexpr_print(fp, LSPREC_CHOICE + 1, indent, echoice->lec_left);

               // Determine adjacent pair around the boundary to pick an operator.
               const lsexpr_t* left_adj = (lsexpr_get_type(echoice->lec_left) == LSETYPE_CHOICE)
                                              ? choice_rightmost_term(lsexpr_get_choice(echoice->lec_left))
                                              : echoice->lec_left;
               const lsexpr_t* right_adj = (lsexpr_get_type(echoice->lec_right) == LSETYPE_CHOICE)
                                               ? choice_leftmost_term(lsexpr_get_choice(echoice->lec_right))
                                               : echoice->lec_right;
               const char* op_sym = (lsexpr_get_type(left_adj) == LSETYPE_LAMBDA &&
                                     lsexpr_get_type(right_adj) == LSETYPE_LAMBDA)
                                        ? " |\n"
                                        : " ||\n";
               lsprintf(fp, indent, "%s", op_sym);

               if (lsexpr_get_type(echoice->lec_right) != LSETYPE_CHOICE)
      break;
    echoice = lsexpr_get_choice(echoice->lec_right);
  }
             // Print the final right term
             lsexpr_print(fp, LSPREC_CHOICE, indent, echoice->lec_right);
             indent--;
             if (prec > LSPREC_CHOICE)
    lsprintf(fp, indent, "\n)");
}
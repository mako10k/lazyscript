
#include "thunk/tchoice.h"
#include "common/malloc.h"
#include "thunk/thunk.h"
#include <assert.h>

struct lstchoice {
  lsthunk_t *ltc_left;
  lsthunk_t *ltc_right;
};

lstchoice_t *lstchoice_new(const lsechoice_t *echoice, lstenv_t *tenv) {
  assert(echoice != NULL);
  lstchoice_t *lchoice = lsmalloc(sizeof(lstchoice_t));
  const lsexpr_t *eleft = lsechoice_get_left(echoice);
  const lsexpr_t *eright = lsechoice_get_right(echoice);
  lsthunk_t *tleft = lsthunk_new_expr(eleft, tenv);
  lsthunk_t *tright = lsthunk_new_expr(eright, tenv);
  lchoice->ltc_left = tleft;
  lchoice->ltc_right = tright;
  return lchoice;
}

lsthunk_t *lstchoice_get_left(const lstchoice_t *choice) {
  assert(choice != NULL);
  return choice->ltc_left;
}

lsthunk_t *lstchoice_get_right(const lstchoice_t *choice) {
  assert(choice != NULL);
  return choice->ltc_right;
}

lsthunk_t *lstchoice_eval(lstchoice_t *choice) {
  assert(choice != NULL);
  lsthunk_t *left = lsthunk_eval(choice->ltc_left);
  if (left != NULL) {
    choice->ltc_left = left;
    return lsthunk_new_choice(choice); // TODO: use original thunk
  }
  return lsthunk_eval(choice->ltc_right);
}

lsthunk_t *lstchoice_apply(lstchoice_t *choice, const lstlist_t *args) {
  assert(choice != NULL);
  assert(args != NULL);
  lsthunk_t *left = lsthunk_apply(choice->ltc_left, args);
  lsthunk_t *right = lsthunk_apply(choice->ltc_right, args);
  if (left == NULL)
    return right;
  if (right == NULL)
    return left;
  choice->ltc_left = left;
  choice->ltc_right = right;
  return lsthunk_new_choice(choice); // TODO: use original thunk
}
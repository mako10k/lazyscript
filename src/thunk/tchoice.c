
#include "thunk/tchoice.h"
#include "common/malloc.h"
#include "thunk/thunk.h"
#include <assert.h>

struct lstchoice {
  lstenv_t *ltc_tenv;
  lsthunk_t *ltc_left;
  lsthunk_t *ltc_right;
};

lstchoice_t *lstchoice_new(lstenv_t *tenv, const lsechoice_t *echoice) {
  assert(tenv != NULL);
  assert(echoice != NULL);
  lstchoice_t *lchoice = lsmalloc(sizeof(lstchoice_t));
  lchoice->ltc_tenv = tenv;
  lchoice->ltc_left = lsthunk_new_expr(tenv, lsechoice_get_left(echoice));
  lchoice->ltc_right = lsthunk_new_expr(tenv, lsechoice_get_right(echoice));
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

lsthunk_t *lstchoice_apply(lstchoice_t *choice, const lstlist_t *args) {
  assert(choice != NULL);
  assert(args != NULL);
  lsthunk_t *left = lsthunk_get_whnf(choice->ltc_left);
  return NULL; // TODO implement
}
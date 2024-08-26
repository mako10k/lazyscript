#include "tappl.h"

struct lstappl {
  lsthunk_t *func;
  lsarray_t *args;
};

lstappl_t *lstappl(lstenv_t *tenv, const lseappl_t *eappl) {
  lstappl_t *tappl = malloc(sizeof(lstappl_t));
  tappl->func = lsexpr_thunk(tenv, lseappl_get_func(eappl));
  tappl->args = lsarray();
  for (unsigned int i = 0; i < lseappl_get_argc(eappl); i++) {
    lsexpr_t *arg = lseappl_get_arg(eappl, i);
    lsarray_push(tappl->args, lsexpr_thunk(tenv, arg));
  }
  return tappl;
}
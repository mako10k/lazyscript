#include "common/const.h"

const lsstr_t *lsconstr_cons(void) {
  static const lsstr_t *str_cons = NULL;
  if (str_cons == NULL)
    str_cons = lsstr_cstr(LSSTR_CONS);
  return str_cons;
}

const lsstr_t *lsconstr_list(void) {
  static const lsstr_t *str_list = NULL;
  if (str_list == NULL)
    str_list = lsstr_cstr("[]");
  return str_list;
}
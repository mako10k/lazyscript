#include "eref.h"
#include "erref.h"
#include "str.h"

struct lseref {
  const lsstr_t *name;
  lserref_t *erref;
};


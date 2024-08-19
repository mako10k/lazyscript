#include "erref.h"
#include "bind.h"
#include "lambda.h"

struct lserref {
  lserrtype_t type;
  union {
    lserrbind_t *bind;
    lserrlambda_t *lambda;
  };
};

struct lserrbind {
  lsbind_t *bind;
};

struct lserrlambda {
  lslambda_t *lambda;
};


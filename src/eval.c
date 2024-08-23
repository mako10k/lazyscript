#include "eval.h"
#include "hash.h"
#include <assert.h>

struct lseval_ctx {
  lshash_t *refs;
  lseval_ctx_t *parent;
};

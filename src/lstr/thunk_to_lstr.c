#include "lstr.h"
#include "thunk/lsti.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include <stdlib.h>

struct lstr_prog { int dummy; };

static lstr_prog_t* lstr_prog_new(void) {
  lstr_prog_t* p = (lstr_prog_t*)malloc(sizeof(struct lstr_prog));
  if (p) p->dummy = 0;
  return p;
}

// Temporary helper: convert LSTI at path to a minimal LSTR program by validating and materializing roots.
// We ignore actual content for now and return an empty program so we can wire CLI and smoke tests.
const lstr_prog_t* lstr_from_lsti_path(const char* path) {
  lsti_image_t img = {0};
  if (lsti_map(path, &img) != 0) return NULL;
  if (lsti_validate(&img) != 0) { lsti_unmap(&img); return NULL; }
  lsthunk_t** roots = NULL; lssize_t rootc = 0;
  lstenv_t* env = lstenv_new(NULL);
  (void)lsti_materialize(&img, &roots, &rootc, env);
  (void)roots; (void)rootc; (void)env;
  lsti_unmap(&img);
  return lstr_prog_new();
}

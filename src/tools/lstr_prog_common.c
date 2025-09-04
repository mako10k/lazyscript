#include "tools/lstr_prog_common.h"
#include <stdio.h>

int lsprog_load_validate(const char* path, const lstr_prog_t** out_prog) {
  if (!out_prog)
    return 1;
  const lstr_prog_t* p = lstr_from_lsti_path(path);
  if (!p) {
    fprintf(stderr, "failed: lstr_from_lsti_path(%s)\n", path ? path : "(null)");
    return 1;
  }
  if (lstr_validate(p) != 0) {
    fprintf(stderr, "invalid LSTR\n");
    return 2;
  }
  *out_prog = p;
  return 0;
}

int lsprog_materialize_roots(const lstr_prog_t* prog,
                             lsthunk_t***       out_roots,
                             lssize_t*          out_rootc,
                             lstenv_t**         out_env) {
  if (!prog || !out_roots || !out_rootc)
    return 3;
  lstenv_t*   env   = lstenv_new(NULL);
  lsthunk_t** roots = NULL;
  lssize_t    rootc = 0;
  if (lstr_materialize_to_thunks(prog, &roots, &rootc, env) != 0) {
    fprintf(stderr, "materialize failed\n");
    return 3;
  }
  if (out_env)
    *out_env = env;
  *out_roots  = roots;
  *out_rootc  = rootc;
  return 0;
}

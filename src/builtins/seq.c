#include "runtime/effects.h"
#include "thunk/thunk.h"
#include "common/str.h"
#include "common/io.h"
#include <stdint.h>

typedef enum lsseq_type { LSSEQ_SIMPLE, LSSEQ_CHAIN } lsseq_type_t;

lsthunk_t* lsbuiltin_seq(lssize_t argc, lsthunk_t* const* args, void* data) {
  (void)argc;
  lsthunk_t* fst = args[0];
  lsthunk_t* snd = args[1];
  ls_effects_begin();
  lsthunk_t* fst_evaled = lsthunk_eval0(fst);
  ls_effects_end();
  if (fst_evaled == NULL)
    return NULL;
  switch ((lsseq_type_t)(intptr_t)data) {
  case LSSEQ_SIMPLE:
    return snd;
  case LSSEQ_CHAIN: {
    lsthunk_t* fst_new =
        lsthunk_new_builtin(lsstr_cstr("seqc"), 2, lsbuiltin_seq, (void*)LSSEQ_CHAIN);
    lsthunk_t* ret = lsthunk_eval(fst_new, 1, &snd);
    return ret;
  }
  }
  return snd;
}

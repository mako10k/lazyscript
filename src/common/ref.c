#include "common/ref.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "lstypes.h"
#include <assert.h>

struct lsref {
  const lsstr_t* ler_name;
  lsloc_t        ler_loc;
};

const lsref_t* lsref_new(const lsstr_t* name, lsloc_t loc) {
  assert(name != NULL);
  lsref_t* ref  = lsmalloc(sizeof(lsref_t));
  ref->ler_name = name;
  ref->ler_loc  = loc;
  return ref;
}

const lsstr_t* lsref_get_name(const lsref_t* ref) {
  assert(ref != NULL);
  return ref->ler_name;
}

lsloc_t lsref_get_loc(const lsref_t* ref) {
  assert(ref != NULL);
  return ref->ler_loc;
}

void lsref_print(FILE* fp, lsprec_t prec, int indent, const lsref_t* ref) {
  assert(fp != NULL);
  assert(ref != NULL);
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, ref->ler_name);
}

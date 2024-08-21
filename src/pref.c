#include "pref.h"
#include "lazyscript.h"
#include "malloc.h"

struct lspref {
  const lsstr_t *name;
};

lspref_t *lspref(const lsstr_t *name) {
  lspref_t *pref = lsmalloc(sizeof(lspref_t));
  pref->name = name;
  return pref;
}

const lsstr_t *lspref_get_name(const lspref_t *ref) { return ref->name; }

void lspref_print(FILE *fp, int prec, int indent, const lspref_t *ref) {
  (void)prec;
  lsprintf(fp, indent, "~");
  lsstr_print_bare(fp, prec, indent, ref->name);
}
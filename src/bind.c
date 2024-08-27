#include "bind.h"
#include "belist.h"
#include "io.h"
#include "malloc.h"
#include <assert.h>

struct lsbind_entry {
  lspat_t *lbe_lhs;
  lsexpr_t *lbe_rhs;
};

lsbind_entry_t *lsbind_entry_new(lspat_t *lhs, lsexpr_t *rhs) {
  lsbind_entry_t *ent = lsmalloc(sizeof(lsbind_entry_t));
  ent->lbe_lhs = lhs;
  ent->lbe_rhs = rhs;
  return ent;
}

void lsbind_entry_print(FILE *fp, lsprec_t prec, int indent,
                        lsbind_entry_t *ent) {
  lspat_print(fp, prec, indent, ent->lbe_lhs);
  lsprintf(fp, indent, " = ");
  lsexpr_print(fp, prec, indent, ent->lbe_rhs);
}

struct lsbind {
  const lsbelist_t *lb_entries;
};

lsbind_t *lsbind_new(void) {
  lsbind_t *bind = lsmalloc(sizeof(lsbind_t));
  bind->lb_entries = lsbelist_new();
  return bind;
}

void lsbind_push(lsbind_t *bind, lsbind_entry_t *ent) {
  assert(bind != NULL);
  assert(ent != NULL);
  bind->lb_entries = lsbelist_push(bind->lb_entries, ent);
}

void lsbind_print(FILE *fp, lsprec_t prec, int indent, lsbind_t *bind) {
  (void)prec;
  for (const lsbelist_t *le = bind->lb_entries; le != NULL;
       le = lsbelist_get_next(le)) {
    lsprintf(fp, indent, ";\n");
    lsbind_entry_print(fp, LSPREC_LOWEST, indent, lsbelist_get(le, 0));
  }
}

lssize_t lsbind_get_entry_count(const lsbind_t *bind) {
  assert(bind != NULL);
  return lsbelist_count(bind->lb_entries);
}

lsbind_entry_t *lsbind_get_entry(const lsbind_t *bind, lssize_t i) {
  assert(bind != NULL);
  return lsbelist_get(bind->lb_entries, i);
}

lspat_t *lsbind_entry_get_lhs(const lsbind_entry_t *ent) {
  assert(ent != NULL);
  return ent->lbe_lhs;
}

lsexpr_t *lsbind_entry_get_rhs(const lsbind_entry_t *ent) {
  assert(ent != NULL);
  return ent->lbe_rhs;
}

const lsbelist_t *lsbind_get_entries(const lsbind_t *bind) {
  assert(bind != NULL);
  return bind->lb_entries;
}
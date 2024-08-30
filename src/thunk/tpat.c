#include "thunk/tpat.h"
#include "common/malloc.h"
#include "pat/pat.h"
#include <assert.h>

struct lstpat {
  lsptype_t lpt_type;
  union {
    lstpalge_t *lpt_alge;
    lstpas_t *lpt_as;
    const lsint_t *lpt_int;
    lstpref_t *lpt_ref;
    const lsstr_t *lpt_str;
  };
};

struct lstpalge {
  const lsstr_t *ltpa_constr;
  lssize_t ltpa_argc;
  lstpat_t *ltpa_args[0];
};

struct lstpas {
  lstpref_t *ltps_ref;
  lstpat_t *ltps_pat;
};

struct lstpref {
  const lsref_t *ltpr_ref;
  lsthunk_t *ltpr_value;
};

lstpalge_t *lstpalge_new(const lsstr_t *constr, lssize_t argc,
                         lstpat_t *args[]) {
  assert(constr != NULL);
  assert(argc >= 0);
  lstpalge_t *palge = lsmalloc(sizeof(lstpalge_t) + argc * sizeof(lstpat_t *));
  palge->ltpa_constr = constr;
  palge->ltpa_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    palge->ltpa_args[i] = args[i];
  return palge;
}

lstpref_t *lstpref_new(const lsref_t *ref) {
  assert(ref != NULL);
  lstpref_t *pref = lsmalloc(sizeof(lstpref_t));
  pref->ltpr_ref = ref;
  pref->ltpr_value = NULL;
  return pref;
}

lstpas_t *lstpas_new(const lsref_t *ref, lstpat_t *pat) {
  assert(ref != NULL);
  assert(pat != NULL);
  lstpas_t *pas = lsmalloc(sizeof(lstpas_t));
  pas->ltps_ref = lstpref_new(ref);
  pas->ltps_pat = pat;
  return pas;
}

lstpat_t *lstpat_new_alge(lstpalge_t *palge) {
  lstpat_t *pat = lsmalloc(sizeof(lstpat_t));
  pat->lpt_type = LSPTYPE_ALGE;
  pat->lpt_alge = palge;
  return pat;
}

lstpat_t *lstpat_new_as(lstpas_t *pas) {
  lstpat_t *pat = lsmalloc(sizeof(lstpat_t));
  pat->lpt_type = LSPTYPE_AS;
  pat->lpt_as = pas;
  return pat;
}

lstpat_t *lstpat_new_int(const lsint_t *val) {
  lstpat_t *pat = lsmalloc(sizeof(lstpat_t));
  pat->lpt_type = LSPTYPE_INT;
  pat->lpt_int = val;
  return pat;
}

lstpat_t *lstpat_new_ref(lstpref_t *pref) {
  lstpat_t *pat = lsmalloc(sizeof(lstpat_t));
  pat->lpt_type = LSPTYPE_REF;
  pat->lpt_ref = pref;
  return pat;
}

lstpat_t *lstpat_new_str(const lsstr_t *val) {
  lstpat_t *pat = lsmalloc(sizeof(lstpat_t));
  pat->lpt_type = LSPTYPE_STR;
  pat->lpt_str = val;
  return pat;
}

lsptype_t lstpat_get_type(const lstpat_t *pat) { return pat->lpt_type; }

const lstpalge_t *lstpat_get_alge(const lstpat_t *pat) { return pat->lpt_alge; }

const lstpas_t *lstpat_get_as(const lstpat_t *pat) { return pat->lpt_as; }

const lsint_t *lstpat_get_int(const lstpat_t *pat) { return pat->lpt_int; }

const lstpref_t *lstpat_get_ref(const lstpat_t *pat) { return pat->lpt_ref; }

const lsstr_t *lstpat_get_str(const lstpat_t *pat) { return pat->lpt_str; }
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
  lstpalge_t *tpalge = lsmalloc(sizeof(lstpalge_t) + argc * sizeof(lstpat_t *));
  tpalge->ltpa_constr = constr;
  tpalge->ltpa_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    tpalge->ltpa_args[i] = args[i];
  return tpalge;
}

lstpref_t *lstpref_new(const lsref_t *ref) {
  lstpref_t *tpref = lsmalloc(sizeof(lstpref_t));
  tpref->ltpr_ref = ref;
  tpref->ltpr_value = NULL;
  return tpref;
}

lstpas_t *lstpas_new(const lsref_t *ref, lstpat_t *tpat) {
  lstpas_t *tpas = lsmalloc(sizeof(lstpas_t));
  tpas->ltps_ref = lstpref_new(ref);
  tpas->ltps_pat = tpat;
  return tpas;
}

lstpat_t *lstpat_new_alge(lstpalge_t *tpalge) {
  lstpat_t *tpat = lsmalloc(sizeof(lstpat_t));
  tpat->lpt_type = LSPTYPE_ALGE;
  tpat->lpt_alge = tpalge;
  return tpat;
}

lstpat_t *lstpat_new_as(lstpas_t *tpas) {
  lstpat_t *tpat = lsmalloc(sizeof(lstpat_t));
  tpat->lpt_type = LSPTYPE_AS;
  tpat->lpt_as = tpas;
  return tpat;
}

lstpat_t *lstpat_new_int(const lsint_t *val) {
  lstpat_t *tpat = lsmalloc(sizeof(lstpat_t));
  tpat->lpt_type = LSPTYPE_INT;
  tpat->lpt_int = val;
  return tpat;
}

lstpat_t *lstpat_new_ref(lstpref_t *tpref) {
  lstpat_t *tpat = lsmalloc(sizeof(lstpat_t));
  tpat->lpt_type = LSPTYPE_REF;
  tpat->lpt_ref = tpref;
  return tpat;
}

lstpat_t *lstpat_new_str(const lsstr_t *val) {
  lstpat_t *tpat = lsmalloc(sizeof(lstpat_t));
  tpat->lpt_type = LSPTYPE_STR;
  tpat->lpt_str = val;
  return tpat;
}

lsptype_t lstpat_get_type(const lstpat_t *tpat) { return tpat->lpt_type; }

const lstpalge_t *lstpat_get_alge(const lstpat_t *tpat) {
  assert(tpat->lpt_type == LSPTYPE_ALGE);
  return tpat->lpt_alge;
}

const lstpas_t *lstpat_get_as(const lstpat_t *tpat) {
  assert(tpat->lpt_type == LSPTYPE_AS);
  return tpat->lpt_as;
}

const lsint_t *lstpat_get_int(const lstpat_t *tpat) {
  assert(tpat->lpt_type == LSPTYPE_INT);
  return tpat->lpt_int;
}

const lstpref_t *lstpat_get_ref(const lstpat_t *tpat) {
  assert(tpat->lpt_type == LSPTYPE_REF);
  return tpat->lpt_ref;
}

const lsstr_t *lstpat_get_str(const lstpat_t *tpat) {
  assert(tpat->lpt_type == LSPTYPE_STR);
  return tpat->lpt_str;
}
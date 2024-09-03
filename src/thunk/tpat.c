#include "thunk/tpat.h"
#include "common/malloc.h"
#include "pat/palge.h"
#include "pat/pas.h"
#include "pat/pat.h"
#include "thunk/thunk.h"
#include <assert.h>
#include <stddef.h>

typedef struct lstpalge lstpalge_t;
typedef struct lstpref lstpref_t;
typedef struct lstpas lstpas_t;

struct lstpalge {
  const lsstr_t *ltpa_constr;
  lssize_t ltpa_argc;
  lstpat_t *ltpa_args[0];
};

struct lstpref {
  const lsref_t *ltpf_ref;
  lsthunk_t *ltpf_refthunk;
};

struct lstpas {
  lstpat_t *ltpa_ref;
  lstpat_t *ltpa_aspattern;
};

struct lstpat {
  lsptype_t type;
  union {
    lstpalge_t ltp_alge;
    lstpref_t ltp_ref;
    const lsint_t *ltp_intval;
    const lsstr_t *ltp_strval;
    lstpas_t ltp_as;
  };
  lsmres_t ltp_res;
};

lstpat_t *lstpat_new_alge(const lsstr_t *constr, lssize_t argc,
                          lstpat_t *const *args) {
  lstpat_t *tpat =
      lsmalloc(lssizeof(lstpat_t, ltp_alge) + argc * sizeof(lstpat_t *));
  tpat->type = LSPTYPE_ALGE;
  tpat->ltp_alge.ltpa_constr = constr;
  tpat->ltp_alge.ltpa_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    tpat->ltp_alge.ltpa_args[i] = args[i];
  return tpat;
}

lstpat_t *lstpat_new_ref(const lsref_t *ref) {
  lstpat_t *tpat = lsmalloc(lssizeof(lstpat_t, ltp_ref));
  tpat->type = LSPTYPE_REF;
  tpat->ltp_ref.ltpf_ref = ref;
  tpat->ltp_ref.ltpf_refthunk = NULL;
  return tpat;
}

lstpat_t *lstpat_new_as(const lsref_t *ref, lstpat_t *aspattern) {
  lstpat_t *tpat = lsmalloc(lssizeof(lstpat_t, ltp_as));
  tpat->type = LSPTYPE_AS;
  tpat->ltp_as.ltpa_ref = lstpat_new_ref(ref);
  tpat->ltp_as.ltpa_aspattern = aspattern;
  return tpat;
}

lstpat_t *lstpat_new_int(const lsint_t *intval) {
  // NOLINTNEXTLINE(bugprone-sizeof-expression)
  lstpat_t *tpat = lsmalloc(lssizeof(lstpat_t, ltp_intval));
  tpat->type = LSPTYPE_INT;
  tpat->ltp_intval = intval;
  return tpat;
}

lstpat_t *lstpat_new_str(const lsstr_t *strval) {
  // NOLINTNEXTLINE(bugprone-sizeof-expression)
  lstpat_t *tpat = lsmalloc(lssizeof(lstpat_t, ltp_strval));
  tpat->type = LSPTYPE_STR;
  tpat->ltp_strval = strval;
  return tpat;
}

lstpat_t *lstpat_new_pat(const lspat_t *pat, lstenv_t *tenv) {
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE: {
    const lspalge_t *palge = lspat_get_alge(pat);
    const lsstr_t *constr = lspalge_get_constr(palge);
    lssize_t argc = lspalge_get_argc(palge);
    const lspat_t *const *pargs = lspalge_get_args(palge);
    lstpat_t *targs[argc];
    for (lssize_t i = 0; i < argc; i++) {
      targs[i] = lstpat_new_pat(pargs[i], tenv);
      if (targs[i] == NULL)
        return NULL;
    }
    return lstpat_new_alge(constr, argc, targs);
  }
  case LSPTYPE_REF: {
    const lsref_t *pref = lspat_get_ref(pat);
    return lstpat_new_ref(pref);
  }
  case LSPTYPE_AS: {
    const lspas_t *pas = lspat_get_as(pat);
    const lsref_t *pref = lspas_get_ref(pas);
    const lspat_t *paspattern = lspas_get_pat(pas);
    lstpat_t *taspattern = lstpat_new_pat(paspattern, tenv);
    return lstpat_new_as(pref, taspattern);
  }
  case LSPTYPE_INT:
    return lstpat_new_int(lspat_get_int(pat));
  case LSPTYPE_STR:
    return lstpat_new_str(lspat_get_str(pat));
  }
}

lsptype_t lstpat_get_type(const lstpat_t *pat) { return pat->type; }

const lsstr_t *lstpat_get_constr(const lstpat_t *pat) {
  assert(pat->type == LSPTYPE_ALGE);
  return pat->ltp_alge.ltpa_constr;
}

lssize_t lstpat_get_argc(const lstpat_t *pat) {
  assert(pat->type == LSPTYPE_ALGE);
  return pat->ltp_alge.ltpa_argc;
}

lstpat_t *const *lstpat_get_args(const lstpat_t *pat) {
  assert(pat->type == LSPTYPE_ALGE);
  return pat->ltp_alge.ltpa_args;
}

lstpat_t *lstpat_get_ref(const lstpat_t *pat) {
  assert(pat->type == LSPTYPE_AS);
  return pat->ltp_as.ltpa_ref;
}

lstpat_t *lstpat_get_aspattern(const lstpat_t *pat) {
  assert(pat->type == LSPTYPE_AS);
  return pat->ltp_as.ltpa_aspattern;
}

void lstpat_set_reftarget(lstpat_t *pat, lsthunk_t *target) {
  assert(pat->type == LSPTYPE_REF);
  assert(pat->ltp_ref.ltpf_refthunk == NULL);
  pat->ltp_ref.ltpf_refthunk = target;
}
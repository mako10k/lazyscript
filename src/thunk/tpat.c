#include "thunk/tpat.h"
#include "common/malloc.h"
#include "thunk/thunk.h"
#include "common/io.h"
#include <assert.h>
#include <stddef.h>

// Internal thunk-side pattern representation
struct lstpat {
  lsptype_t ltp_type;
  union {
    struct {
      const lsstr_t *constr;
      lssize_t argc;
      lstpat_t *args[0];
    } alge;
    struct {
      lstpat_t *ref;         // left of '@'
      lstpat_t *aspattern;   // right of '@'
    } as;
    const lsint_t *intval;
    const lsstr_t *strval;
    struct {
      const lsref_t *ref;
      lsthunk_t *bound; // set when matched
    } r;
  };
};

static lstpat_t *lstpat_new_alge_internal(const lsstr_t *constr, lssize_t argc,
                                          lstpat_t *const *args) {
  lstpat_t *pat =
      lsmalloc(offsetof(lstpat_t, alge.args) + sizeof(lstpat_t *) * argc);
  pat->ltp_type = LSPTYPE_ALGE;
  pat->alge.constr = constr;
  pat->alge.argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    pat->alge.args[i] = args[i];
  return pat;
}

static lstpat_t *lstpat_from_lspalge(const lspalge_t *palge, lstenv_t *tenv,
                                     lstref_target_origin_t *origin) {
  (void)tenv;
  (void)origin;
  lssize_t argc = lspalge_get_argc(palge);
  lstpat_t *args[argc];
  for (lssize_t i = 0; i < argc; i++) {
    args[i] = lstpat_new_pat(lspalge_get_arg(palge, i), tenv, origin);
    if (args[i] == NULL)
      return NULL;
  }
  return lstpat_new_alge_internal(lspalge_get_constr(palge), argc, args);
}

static lstpat_t *lstpat_from_lspas(const lspas_t *pas, lstenv_t *tenv,
                                   lstref_target_origin_t *origin) {
  lstpat_t *pref = lstpat_new_ref(lspas_get_ref(pas));
  lstpat_t *ppat = lstpat_new_pat(lspas_get_pat(pas), tenv, origin);
  if (ppat == NULL)
    return NULL;
  lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
  ret->ltp_type = LSPTYPE_AS;
  ret->as.ref = pref;
  ret->as.aspattern = ppat;
  return ret;
}

lstpat_t *lstpat_new_pat(const lspat_t *pat, lstenv_t *tenv,
                         lstref_target_origin_t *origin) {
  assert(pat != NULL);
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE:
    return lstpat_from_lspalge(lspat_get_alge(pat), tenv, origin);
  case LSPTYPE_AS:
    return lstpat_from_lspas(lspat_get_as(pat), tenv, origin);
  case LSPTYPE_INT: {
    lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_INT;
    ret->intval = lspat_get_int(pat);
    return ret;
  }
  case LSPTYPE_STR: {
    lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_STR;
    ret->strval = lspat_get_str(pat);
    return ret;
  }
  case LSPTYPE_REF: {
    const lsref_t *ref = lspat_get_ref(pat);
    lstpat_t *p = lstpat_new_ref(ref);
    if (tenv != NULL && origin != NULL) {
      lstref_target_t *target = lstref_target_new(origin, p);
      lstenv_put(tenv, lsref_get_name(ref), target);
    }
    return p;
  }
  }
  return NULL;
}

lstpat_t *lstpat_new_ref(const lsref_t *ref) {
  lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
  ret->ltp_type = LSPTYPE_REF;
  ret->r.ref = ref;
  ret->r.bound = NULL;
  return ret;
}

lsptype_t lstpat_get_type(const lstpat_t *pat) { return pat->ltp_type; }

const lsstr_t *lstpat_get_constr(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_ALGE);
  return pat->alge.constr;
}

lssize_t lstpat_get_argc(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_ALGE);
  return pat->alge.argc;
}

lstpat_t *const *lstpat_get_args(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_ALGE);
  return pat->alge.args;
}

lstpat_t *lstpat_get_ref(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_AS);
  return pat->as.ref;
}

lstpat_t *lstpat_get_aspattern(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_AS);
  return pat->as.aspattern;
}

const lsint_t *lstpat_get_int(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_INT);
  return pat->intval;
}

const lsstr_t *lstpat_get_str(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_STR);
  return pat->strval;
}

void lstpat_set_refbound(lstpat_t *pat, lsthunk_t *thunk) {
  assert(pat->ltp_type == LSPTYPE_REF);
  pat->r.bound = thunk;
}

lsthunk_t *lstpat_get_refbound(const lstpat_t *pat) {
  assert(pat->ltp_type == LSPTYPE_REF);
  return pat->r.bound;
}

static void lstpat_clear_binds_internal(lstpat_t *pat) {
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE:
    for (lssize_t i = 0; i < pat->alge.argc; i++)
      lstpat_clear_binds_internal(pat->alge.args[i]);
    break;
  case LSPTYPE_AS:
    lstpat_clear_binds_internal(pat->as.ref);
    lstpat_clear_binds_internal(pat->as.aspattern);
    break;
  case LSPTYPE_INT:
  case LSPTYPE_STR:
    break;
  case LSPTYPE_REF:
    pat->r.bound = NULL;
    break;
  }
}

void lstpat_clear_binds(lstpat_t *pat) { lstpat_clear_binds_internal(pat); }

static lstpat_t *lstpat_clone_internal(const lstpat_t *pat) {
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE: {
    lssize_t argc = pat->alge.argc;
    lstpat_t *args[argc];
    for (lssize_t i = 0; i < argc; i++)
      args[i] = lstpat_clone_internal(pat->alge.args[i]);
    return lstpat_new_alge_internal(pat->alge.constr, argc, args);
  }
  case LSPTYPE_AS: {
    lstpat_t *pref = lstpat_clone_internal(pat->as.ref);
    lstpat_t *ppat = lstpat_clone_internal(pat->as.aspattern);
    lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_AS;
    ret->as.ref = pref;
    ret->as.aspattern = ppat;
    return ret;
  }
  case LSPTYPE_INT: {
    lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_INT;
    ret->intval = pat->intval;
    return ret;
  }
  case LSPTYPE_STR: {
    lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_STR;
    ret->strval = pat->strval;
    return ret;
  }
  case LSPTYPE_REF: {
    lstpat_t *ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_REF;
    ret->r.ref = pat->r.ref;
    ret->r.bound = pat->r.bound;
    return ret;
  }
  }
  return NULL;
}

lstpat_t *lstpat_clone(const lstpat_t *pat) { return lstpat_clone_internal(pat); }

void lstpat_print(FILE *fp, lsprec_t prec, int indent, const lstpat_t *pat) {
  // Use the AST pattern printer as a baseline by converting back.
  // For now, implement a simple printer mirroring thunk.h usage.
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE: {
    if (pat->alge.argc == 0) {
      lsstr_print_bare(fp, prec, indent, pat->alge.constr);
      return;
    }
    if (prec > LSPREC_APPL)
      lsprintf(fp, indent, "(");
    lsstr_print_bare(fp, prec, indent, pat->alge.constr);
    for (lssize_t i = 0; i < pat->alge.argc; i++) {
      lsprintf(fp, indent, " ");
      lstpat_print(fp, LSPREC_APPL + 1, indent, pat->alge.args[i]);
    }
    if (prec > LSPREC_APPL)
      lsprintf(fp, 0, ")");
    break;
  }
  case LSPTYPE_AS:
    if (prec > LSPREC_LAMBDA)
      lsprintf(fp, indent, "(");
    lstpat_print(fp, LSPREC_APPL + 1, indent, pat->as.ref);
    lsprintf(fp, indent, " @ ");
    lstpat_print(fp, LSPREC_LAMBDA, indent, pat->as.aspattern);
    if (prec > LSPREC_LAMBDA)
      lsprintf(fp, indent, ")");
    break;
  case LSPTYPE_INT:
    lsint_print(fp, prec, indent, pat->intval);
    break;
  case LSPTYPE_STR:
    lsstr_print(fp, prec, indent, pat->strval);
    break;
  case LSPTYPE_REF:
    lsref_print(fp, prec, indent, pat->r.ref);
    break;
  }
}

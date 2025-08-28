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
      const lsstr_t* constr;
      lssize_t       argc;
      lstpat_t*      args[0];
    } alge;
    struct {
      lstpat_t* ref;       // left of '@'
      lstpat_t* aspattern; // right of '@'
    } as;
    const lsint_t* intval;
    const lsstr_t* strval;
    struct {
      const lsref_t* ref;
      lsthunk_t*     bound; // set when matched
    } r;
    struct {
      int wild;
    } w;
    struct {
      lstpat_t* left;
      lstpat_t* right;
    } orp;
    struct {
      lstpat_t* inner;
    } caret;
  };
};

// Sentinels for temporary mark during OR var-set validation
static int              ls_or_mark_present_s;
static int              ls_or_mark_used_s;
static lsthunk_t* const ls_or_mark_present = (lsthunk_t*)&ls_or_mark_present_s;
static lsthunk_t* const ls_or_mark_used    = (lsthunk_t*)&ls_or_mark_used_s;
static int              ls_or_newvar_seen  = 0;

// Forward decls for internal helpers
static void      lstpat_mark_refs_present(lstpat_t* pat);
static void      lstpat_mark_clear(lstpat_t* pat);
static void      lstpat_mark_ref_used_by_name(const lsstr_t* name, lstenv_t* tenv);
static int       lstpat_any_mark_present(const lstpat_t* pat);
static lstpat_t* lstpat_new_pat_reuse_only(const lspat_t* pat, lstenv_t* tenv,
                                           lstref_target_origin_t* origin);
static void      lspat_walk_mark_right(const lspat_t* p, lstenv_t* tenv);

static void      lspat_walk_mark_right(const lspat_t* p, lstenv_t* tenv) {
       switch (lspat_get_type(p)) {
       case LSPTYPE_ALGE: {
         const lspalge_t* pa   = lspat_get_alge(p);
         lssize_t         argc = lspalge_get_argc(pa);
         for (lssize_t i = 0; i < argc; i++)
      lspat_walk_mark_right(lspalge_get_arg(pa, i), tenv);
    break;
  }
       case LSPTYPE_AS: {
         const lspas_t* pa = lspat_get_as(p);
         lspat_walk_mark_right((const lspat_t*)lspas_get_pat(pa), tenv);
         const lsref_t* r = lspas_get_ref(pa);
         lstpat_mark_ref_used_by_name(lsref_get_name(r), tenv);
         break;
  }
       case LSPTYPE_INT:
       case LSPTYPE_STR:
       case LSPTYPE_WILDCARD:
    break;
       case LSPTYPE_REF: {
         const lsref_t* r = lspat_get_ref(p);
         lstpat_mark_ref_used_by_name(lsref_get_name(r), tenv);
         break;
  }
       case LSPTYPE_OR:
    lspat_walk_mark_right(lspat_get_or_left(p), tenv);
    lspat_walk_mark_right(lspat_get_or_right(p), tenv);
    break;
  case LSPTYPE_CARET:
    lspat_walk_mark_right(lspat_get_caret_inner(p), tenv);
    break;
  }
}

static lstpat_t* lstpat_new_alge_internal(const lsstr_t* constr, lssize_t argc,
                                          lstpat_t* const* args) {
  lstpat_t* pat    = lsmalloc(offsetof(lstpat_t, alge.args) + sizeof(lstpat_t*) * argc);
  pat->ltp_type    = LSPTYPE_ALGE;
  pat->alge.constr = constr;
  pat->alge.argc   = argc;
  for (lssize_t i = 0; i < argc; i++)
    pat->alge.args[i] = args[i];
  return pat;
}

static lstpat_t* lstpat_from_lspalge(const lspalge_t* palge, lstenv_t* tenv,
                                     lstref_target_origin_t* origin) {
  (void)tenv;
  (void)origin;
  lssize_t  argc = lspalge_get_argc(palge);
  if (argc == 0) {
    return lstpat_new_alge_internal(lspalge_get_constr(palge), 0, NULL);
  } else {
    lstpat_t* args[argc];
    for (lssize_t i = 0; i < argc; i++) {
      args[i] = lstpat_new_pat(lspalge_get_arg(palge, i), tenv, origin);
      if (args[i] == NULL)
        return NULL;
    }
    return lstpat_new_alge_internal(lspalge_get_constr(palge), argc, args);
  }
}

static lstpat_t* lstpat_from_lspas(const lspas_t* pas, lstenv_t* tenv,
                                   lstref_target_origin_t* origin) {
  lstpat_t* pref = lstpat_new_ref(lspas_get_ref(pas));
  lstpat_t* ppat = lstpat_new_pat(lspas_get_pat(pas), tenv, origin);
  if (ppat == NULL)
    return NULL;
  lstpat_t* ret     = lsmalloc(sizeof(lstpat_t));
  ret->ltp_type     = LSPTYPE_AS;
  ret->as.ref       = pref;
  ret->as.aspattern = ppat;
  return ret;
}

lstpat_t* lstpat_new_pat(const lspat_t* pat, lstenv_t* tenv, lstref_target_origin_t* origin) {
  assert(pat != NULL);
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE:
    return lstpat_from_lspalge(lspat_get_alge(pat), tenv, origin);
  case LSPTYPE_AS:
    return lstpat_from_lspas(lspat_get_as(pat), tenv, origin);
  case LSPTYPE_CARET: {
    lstpat_t* in = lstpat_new_pat(lspat_get_caret_inner(pat), tenv, origin);
    if (!in) return NULL;
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_CARET;
    ret->caret.inner = in;
    return ret;
  }
  case LSPTYPE_OR: {
    // For OR, build left first, then enforce right uses exactly the same
    // variable set; reuse the same ref nodes via env (no new names allowed).
    const lspat_t* pleft  = lspat_get_or_left(pat);
    const lspat_t* pright = lspat_get_or_right(pat);
    lstpat_t*      l      = lstpat_new_pat(pleft, tenv, origin);
    if (!l)
      return NULL;

    // Mark all ref nodes from left as "present"
    lstpat_mark_refs_present(l);

    // Walk right AST to ensure it introduces no new names and marks all as used
    ls_or_newvar_seen = 0;
    lspat_walk_mark_right(pright, tenv);
    if (ls_or_newvar_seen) {
      // clear marks before returning
      lstpat_mark_clear(l);
      return NULL;
    }

    // If any left-mark is still present, right missed usage
    if (lstpat_any_mark_present(l)) {
      if (tenv)
        lstenv_incr_nerrors(tenv);
      lsprintf(stderr, 0,
               "E: right side of pattern '|' must use all variables bound on the left (use '_' if "
               "you want to ignore some)\n");
      // clear marks before returning
      lstpat_mark_clear(l);
      return NULL;
    }

    // Build right, reusing existing names only
    lstpat_t* r = lstpat_new_pat_reuse_only(pright, tenv, origin);
    if (!r) {
      // clear marks before returning
      lstpat_mark_clear(l);
      return NULL;
    }
    // clear temporary marks
    lstpat_mark_clear(l);
    lstpat_t* ret  = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type  = LSPTYPE_OR;
    ret->orp.left  = l;
    ret->orp.right = r;
    return ret;
  }
  case LSPTYPE_INT: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_INT;
    ret->intval   = lspat_get_int(pat);
    return ret;
  }
  case LSPTYPE_STR: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_STR;
    ret->strval   = lspat_get_str(pat);
    return ret;
  }
  case LSPTYPE_REF: {
    const lsref_t* ref = lspat_get_ref(pat);
    if (tenv != NULL && origin != NULL) {
      // Reuse existing name in current scope if present; else create new
      lstref_target_t* existing = lstenv_get_self(tenv, lsref_get_name(ref));
      if (existing != NULL) {
        return lstref_target_get_pat(existing);
      }
      lstpat_t*        p      = lstpat_new_ref(ref);
      lstref_target_t* target = lstref_target_new(origin, p);
      lstenv_put(tenv, lsref_get_name(ref), target);
      return p;
    } else {
      return lstpat_new_ref(ref);
    }
  }
  case LSPTYPE_WILDCARD: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_WILDCARD;
    ret->w.wild   = 1;
    return ret;
  }
  }
  return NULL;
}

lstpat_t* lstpat_new_ref(const lsref_t* ref) {
  lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
  ret->ltp_type = LSPTYPE_REF;
  ret->r.ref    = ref;
  ret->r.bound  = NULL;
  return ret;
}

lsptype_t      lstpat_get_type(const lstpat_t* pat) { return pat->ltp_type; }

const lsstr_t* lstpat_get_constr(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_ALGE);
  return pat->alge.constr;
}

lssize_t lstpat_get_argc(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_ALGE);
  return pat->alge.argc;
}

lstpat_t* const* lstpat_get_args(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_ALGE);
  return pat->alge.args;
}

lstpat_t* lstpat_get_ref(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_AS);
  return pat->as.ref;
}

lstpat_t* lstpat_get_aspattern(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_AS);
  return pat->as.aspattern;
}

const lsint_t* lstpat_get_int(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_INT);
  return pat->intval;
}

const lsstr_t* lstpat_get_str(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_STR);
  return pat->strval;
}

int  lstpat_is_wild(const lstpat_t* pat) { return pat->ltp_type == LSPTYPE_WILDCARD; }

void lstpat_set_refbound(lstpat_t* pat, lsthunk_t* thunk) {
  assert(pat->ltp_type == LSPTYPE_REF);
  pat->r.bound = thunk;
}

lsthunk_t* lstpat_get_refbound(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_REF);
  return pat->r.bound;
}

lstpat_t* lstpat_get_or_left(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_OR);
  return pat->orp.left;
}

lstpat_t* lstpat_get_or_right(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_OR);
  return pat->orp.right;
}

static void lstpat_clear_binds_internal(lstpat_t* pat) {
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
  case LSPTYPE_WILDCARD:
    break;
  case LSPTYPE_OR:
    lstpat_clear_binds_internal(pat->orp.left);
    lstpat_clear_binds_internal(pat->orp.right);
    break;
  case LSPTYPE_CARET:
    lstpat_clear_binds_internal(pat->caret.inner);
    break;
  }
}

void             lstpat_clear_binds(lstpat_t* pat) { lstpat_clear_binds_internal(pat); }

static lstpat_t* lstpat_clone_internal(const lstpat_t* pat) {
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE: {
    lssize_t argc = pat->alge.argc;
    if (argc == 0) {
      return lstpat_new_alge_internal(pat->alge.constr, 0, NULL);
    } else {
      lstpat_t* args[argc];
      for (lssize_t i = 0; i < argc; i++)
        args[i] = lstpat_clone_internal(pat->alge.args[i]);
      return lstpat_new_alge_internal(pat->alge.constr, argc, args);
    }
  }
  case LSPTYPE_AS: {
    lstpat_t* pref    = lstpat_clone_internal(pat->as.ref);
    lstpat_t* ppat    = lstpat_clone_internal(pat->as.aspattern);
    lstpat_t* ret     = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type     = LSPTYPE_AS;
    ret->as.ref       = pref;
    ret->as.aspattern = ppat;
    return ret;
  }
  case LSPTYPE_INT: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_INT;
    ret->intval   = pat->intval;
    return ret;
  }
  case LSPTYPE_STR: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_STR;
    ret->strval   = pat->strval;
    return ret;
  }
  case LSPTYPE_REF: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_REF;
    ret->r.ref    = pat->r.ref;
    ret->r.bound  = pat->r.bound;
    return ret;
  }
  case LSPTYPE_WILDCARD: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_WILDCARD;
    ret->w.wild   = 1;
    return ret;
  }
  case LSPTYPE_OR: {
    lstpat_t* ret  = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type  = LSPTYPE_OR;
    ret->orp.left  = lstpat_clone_internal(pat->orp.left);
    ret->orp.right = lstpat_clone_internal(pat->orp.right);
    return ret;
  }
  case LSPTYPE_CARET: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_CARET;
    ret->caret.inner = lstpat_clone_internal(pat->caret.inner);
    return ret;
  }
  }
  return NULL;
}

lstpat_t* lstpat_clone(const lstpat_t* pat) { return lstpat_clone_internal(pat); }

void      lstpat_print(FILE* fp, lsprec_t prec, int indent, const lstpat_t* pat) {
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
  case LSPTYPE_WILDCARD:
    lsprintf(fp, indent, "_");
    break;
  case LSPTYPE_OR:
    if (prec > LSPREC_CHOICE)
      lsprintf(fp, indent, "(");
    lstpat_print(fp, LSPREC_CHOICE + 1, indent, pat->orp.left);
    lsprintf(fp, indent, " | ");
    lstpat_print(fp, LSPREC_CHOICE, indent, pat->orp.right);
    if (prec > LSPREC_CHOICE)
      lsprintf(fp, indent, ")");
    break;
  case LSPTYPE_CARET:
    lsprintf(fp, indent, "^(" );
    lstpat_print(fp, LSPREC_LOWEST, indent, pat->caret.inner);
    lsprintf(fp, indent, ")");
    break;
  }
}

// Public accessor for caret inner
lstpat_t* lstpat_get_caret_inner(const lstpat_t* pat) {
  assert(pat->ltp_type == LSPTYPE_CARET);
  return pat->caret.inner;
}

// Helpers implementation
static void lstpat_mark_refs_present(lstpat_t* pat) {
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE:
    for (lssize_t i = 0; i < pat->alge.argc; i++)
      lstpat_mark_refs_present(pat->alge.args[i]);
    break;
  case LSPTYPE_AS:
    lstpat_mark_refs_present(pat->as.ref);
    lstpat_mark_refs_present(pat->as.aspattern);
    break;
  case LSPTYPE_REF:
    pat->r.bound = ls_or_mark_present;
    break;
  case LSPTYPE_OR:
    lstpat_mark_refs_present(pat->orp.left);
    lstpat_mark_refs_present(pat->orp.right);
    break;
  case LSPTYPE_INT:
  case LSPTYPE_STR:
  case LSPTYPE_WILDCARD:
    break;
  case LSPTYPE_CARET:
    lstpat_mark_refs_present(pat->caret.inner);
    break;
  }
}

static void lstpat_mark_ref_used_by_name(const lsstr_t* name, lstenv_t* tenv) {
  if (!tenv)
    return;
  lstref_target_t* target = lstenv_get_self(tenv, name);
  if (target == NULL) {
    // new var introduced on right: error is reported by reuse-only builder later
    lstenv_incr_nerrors(tenv);
    lsprintf(stderr, 0, "E: right side of pattern '|' introduces new variable '");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
    lsprintf(stderr, 0, "' (not allowed)\n");
    ls_or_newvar_seen = 1;
    return;
  }
  lstpat_t* pref = lstref_target_get_pat(target);
  if (pref->ltp_type == LSPTYPE_REF && pref->r.bound == ls_or_mark_present)
    pref->r.bound = ls_or_mark_used;
}

static int lstpat_any_mark_present(const lstpat_t* pat) {
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE:
    for (lssize_t i = 0; i < pat->alge.argc; i++)
      if (lstpat_any_mark_present(pat->alge.args[i]))
        return 1;
    return 0;
  case LSPTYPE_AS:
    return lstpat_any_mark_present(pat->as.ref) || lstpat_any_mark_present(pat->as.aspattern);
  case LSPTYPE_REF:
    return pat->r.bound == ls_or_mark_present;
  case LSPTYPE_OR:
    return lstpat_any_mark_present(pat->orp.left) || lstpat_any_mark_present(pat->orp.right);
  case LSPTYPE_INT:
  case LSPTYPE_STR:
  case LSPTYPE_WILDCARD:
    return 0;
  case LSPTYPE_CARET:
    return lstpat_any_mark_present(pat->caret.inner);
  }
  return 0;
}

static void lstpat_mark_clear(lstpat_t* pat) {
  switch (pat->ltp_type) {
  case LSPTYPE_ALGE:
    for (lssize_t i = 0; i < pat->alge.argc; i++)
      lstpat_mark_clear(pat->alge.args[i]);
    break;
  case LSPTYPE_AS:
    lstpat_mark_clear(pat->as.ref);
    lstpat_mark_clear(pat->as.aspattern);
    break;
  case LSPTYPE_REF:
    if (pat->r.bound == ls_or_mark_present || pat->r.bound == ls_or_mark_used)
      pat->r.bound = NULL;
    break;
  case LSPTYPE_OR:
    lstpat_mark_clear(pat->orp.left);
    lstpat_mark_clear(pat->orp.right);
    break;
  case LSPTYPE_INT:
  case LSPTYPE_STR:
  case LSPTYPE_WILDCARD:
    break;
  case LSPTYPE_CARET:
    lstpat_mark_clear(pat->caret.inner);
    break;
  }
}

static lstpat_t* lstpat_new_pat_reuse_only(const lspat_t* pat, lstenv_t* tenv,
                                           lstref_target_origin_t* origin) {
  (void)origin;
  assert(pat != NULL);
  switch (lspat_get_type(pat)) {
  case LSPTYPE_ALGE: {
    const lspalge_t* pa   = lspat_get_alge(pat);
    lssize_t         argc = lspalge_get_argc(pa);
    if (argc == 0) {
      return lstpat_new_alge_internal(lspalge_get_constr(pa), 0, NULL);
    } else {
      lstpat_t* args[argc];
      for (lssize_t i = 0; i < argc; i++) {
        args[i] = lstpat_new_pat_reuse_only(lspalge_get_arg(pa, i), tenv, origin);
        if (!args[i])
          return NULL;
      }
      return lstpat_new_alge_internal(lspalge_get_constr(pa), argc, args);
    }
  }
  case LSPTYPE_CARET: {
    lstpat_t* in = lstpat_new_pat_reuse_only(lspat_get_caret_inner(pat), tenv, origin);
    if (!in) return NULL;
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_CARET;
    ret->caret.inner = in;
    return ret;
  }
  case LSPTYPE_AS: {
    const lspas_t*   pa       = lspat_get_as(pat);
    const lsref_t*   r        = lspas_get_ref(pa);
    lstref_target_t* existing = lstenv_get_self(tenv, lsref_get_name(r));
    if (!existing) {
      if (tenv)
        lstenv_incr_nerrors(tenv);
      lsprintf(stderr, 0, "E: right side of pattern '|' introduces new variable '");
      lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(r));
      lsprintf(stderr, 0, "' (not allowed)\n");
      return NULL;
    }
    lstpat_t* pref = lstref_target_get_pat(existing);
    lstpat_t* ppat = lstpat_new_pat_reuse_only(lspas_get_pat(pa), tenv, origin);
    if (!ppat)
      return NULL;
    lstpat_t* ret     = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type     = LSPTYPE_AS;
    ret->as.ref       = pref;
    ret->as.aspattern = ppat;
    return ret;
  }
  case LSPTYPE_INT: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_INT;
    ret->intval   = lspat_get_int(pat);
    return ret;
  }
  case LSPTYPE_STR: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_STR;
    ret->strval   = lspat_get_str(pat);
    return ret;
  }
  case LSPTYPE_REF: {
    const lsref_t*   r        = lspat_get_ref(pat);
    lstref_target_t* existing = lstenv_get_self(tenv, lsref_get_name(r));
    if (!existing) {
      if (tenv)
        lstenv_incr_nerrors(tenv);
      lsprintf(stderr, 0, "E: right side of pattern '|' introduces new variable '");
      lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(r));
      lsprintf(stderr, 0, "' (not allowed)\n");
      return NULL;
    }
    return lstref_target_get_pat(existing);
  }
  case LSPTYPE_WILDCARD: {
    lstpat_t* ret = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type = LSPTYPE_WILDCARD;
    ret->w.wild   = 1;
    return ret;
  }
  case LSPTYPE_OR: {
    // Build both arms under reuse-only constraint
    lstpat_t* l = lstpat_new_pat_reuse_only(lspat_get_or_left(pat), tenv, origin);
    if (!l)
      return NULL;
    lstpat_t* r = lstpat_new_pat_reuse_only(lspat_get_or_right(pat), tenv, origin);
    if (!r)
      return NULL;
    lstpat_t* ret  = lsmalloc(sizeof(lstpat_t));
    ret->ltp_type  = LSPTYPE_OR;
    ret->orp.left  = l;
    ret->orp.right = r;
    return ret;
  }
  }
  return NULL;
}

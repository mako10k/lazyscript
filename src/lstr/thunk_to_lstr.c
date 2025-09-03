#include "lstr.h"
#include "lstr_internal.h"
#include "thunk/lsti.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"
#include "common/int.h"
#include "common/str.h"
#include "thunk/thunk_bin.h"
#include <stdlib.h>
#include <string.h>

static lstr_expr_t* to_expr_from_thunk(lsthunk_t* t);
static lstr_pat_t*  to_pat_from_lstpat(lstpat_t* p);
static lstr_pat_t*  new_pat(lstr_pat_kind_t k) {
   lstr_pat_t* p = (lstr_pat_t*)calloc(1, sizeof(lstr_pat_t));
   if (p)
    p->kind = k;
  return p;
}

static lstr_pat_t* to_pat_from_lstpat(lstpat_t* p) {
  if (!p)
    return new_pat(LSTR_PAT_WILDCARD);
  switch (lstpat_get_type(p)) {
  case LSPTYPE_ALGE: {
    lstr_pat_t* q     = new_pat(LSTR_PAT_CONSTR);
    q->as.constr.name = lstpat_get_constr(p);
    lssize_t argc     = lstpat_get_argc(p);
    q->as.constr.argc = (size_t)argc;
    if (argc > 0) {
      q->as.constr.args     = (lstr_pat_t**)calloc((size_t)argc, sizeof(lstr_pat_t*));
      lstpat_t* const* args = lstpat_get_args(p);
      for (lssize_t i = 0; i < argc; i++)
        q->as.constr.args[i] = to_pat_from_lstpat(args[i]);
    }
    return q;
  }
  case LSPTYPE_AS: {
    lstr_pat_t*    q    = new_pat(LSTR_PAT_AS);
    lstpat_t*      refp = lstpat_get_ref(p);
    const lsstr_t* rn   = lstpat_get_refname(refp);
    if (rn) {
      q->as.as_pat.ref_pat              = new_pat(LSTR_PAT_VAR);
      q->as.as_pat.ref_pat->as.var_name = rn;
    } else {
      q->as.as_pat.ref_pat = to_pat_from_lstpat(refp);
    }
    q->as.as_pat.inner = to_pat_from_lstpat(lstpat_get_aspattern(p));
    return q;
  }
  case LSPTYPE_INT: {
    lstr_pat_t* q = new_pat(LSTR_PAT_INT);
    q->as.i       = (long long)lsint_get(lstpat_get_int(p));
    return q;
  }
  case LSPTYPE_STR: {
    lstr_pat_t* q = new_pat(LSTR_PAT_STR);
    q->as.s       = lstpat_get_str(p);
    return q;
  }
  case LSPTYPE_REF: {
    lstr_pat_t* q  = new_pat(LSTR_PAT_VAR);
    q->as.var_name = lstpat_get_refname(p);
    return q;
  }
  case LSPTYPE_WILDCARD:
    return new_pat(LSTR_PAT_WILDCARD);
  case LSPTYPE_OR: {
    lstr_pat_t* q      = new_pat(LSTR_PAT_OR);
    q->as.or_pat.left  = to_pat_from_lstpat(lstpat_get_or_left(p));
    q->as.or_pat.right = to_pat_from_lstpat(lstpat_get_or_right(p));
    return q;
  }
  case LSPTYPE_CARET: {
    lstr_pat_t* q     = new_pat(LSTR_PAT_CARET);
    q->as.caret_inner = to_pat_from_lstpat(lstpat_get_caret_inner(p));
    return q;
  }
  default:
    return new_pat(LSTR_PAT_WILDCARD);
  }
}

static lstr_val_t* new_val(lstr_val_kind_t k) {
  lstr_val_t* v = (lstr_val_t*)calloc(1, sizeof(lstr_val_t));
  if (v)
    v->kind = k;
  return v;
}
static lstr_expr_t* new_expr(lstr_exp_kind_t k) {
  lstr_expr_t* e = (lstr_expr_t*)calloc(1, sizeof(lstr_expr_t));
  if (e)
    e->kind = k;
  return e;
}

static lstr_val_t* to_val_from_thunk(lsthunk_t* t) {
  switch (lsthunk_get_type(t)) {
  case LSTTYPE_INT: {
    lstr_val_t*    v = new_val(LSTR_VAL_INT);
    const lsint_t* i = lsthunk_get_int(t);
    v->as.i          = (long long)lsint_get(i);
    return v;
  }
  case LSTTYPE_STR: {
    lstr_val_t* v = new_val(LSTR_VAL_STR);
    v->as.s       = lsthunk_get_str(t);
    return v;
  }
  case LSTTYPE_SYMBOL: {
    lstr_val_t* v  = new_val(LSTR_VAL_REF);
    v->as.ref_name = lsthunk_get_symbol(t);
    return v;
  }
  case LSTTYPE_REF: {
    lstr_val_t* v  = new_val(LSTR_VAL_REF);
    v->as.ref_name = lsthunk_get_ref_name(t);
    return v;
  }
  case LSTTYPE_ALGE: {
    lstr_val_t* v     = new_val(LSTR_VAL_CONSTR);
    v->as.constr.name = lsthunk_get_constr(t);
    lssize_t argc     = lsthunk_get_argc(t);
    v->as.constr.argc = (size_t)argc;
    if (argc > 0) {
      v->as.constr.args      = (lstr_val_t**)calloc((size_t)argc, sizeof(lstr_val_t*));
      lsthunk_t* const* args = lsthunk_get_args(t);
      for (lssize_t i = 0; i < argc; i++)
        v->as.constr.args[i] = to_val_from_thunk(args[i]);
    }
    return v;
  }
  case LSTTYPE_CHOICE: {
    lstr_val_t* v      = new_val(LSTR_VAL_CHOICE);
    v->as.choice.kind  = lsthunk_get_choice_kind(t);
    v->as.choice.left  = to_expr_from_thunk(lsthunk_get_choice_left(t));
    v->as.choice.right = to_expr_from_thunk(lsthunk_get_choice_right(t));
    return v;
  }
  case LSTTYPE_BUILTIN: {
    lstr_val_t* v  = new_val(LSTR_VAL_REF);
    v->as.ref_name = lsthunk_get_builtin_name(t);
    return v;
  }
  case LSTTYPE_LAMBDA: {
    lstr_val_t* v   = new_val(LSTR_VAL_LAM);
    lstpat_t*   p   = lsthunk_get_param(t);
    v->as.lam.param = to_pat_from_lstpat(p);
    v->as.lam.body  = to_expr_from_thunk(lsthunk_get_body(t));
    return v;
  }
  case LSTTYPE_BOTTOM: {
    lstr_val_t* v     = new_val(LSTR_VAL_BOTTOM);
    v->as.bottom.msg  = lsthunk_bottom_get_message(t);
    lssize_t rc       = lsthunk_bottom_get_argc(t);
    v->as.bottom.relc = (size_t)rc;
    if (rc > 0) {
      v->as.bottom.related  = (lstr_val_t**)calloc((size_t)rc, sizeof(lstr_val_t*));
      lsthunk_t* const* rel = lsthunk_bottom_get_args(t);
      for (lssize_t i = 0; i < rc; i++)
        v->as.bottom.related[i] = to_val_from_thunk(rel[i]);
    }
    return v;
  }
  default:
    return new_val(LSTR_VAL_BOTTOM); // placeholder for unsupported kinds
  }
}

static lstr_expr_t* to_expr_from_thunk(lsthunk_t* t) {
  if (lsthunk_get_type(t) == LSTTYPE_APPL) {
    lstr_expr_t* e = new_expr(LSTR_EXP_APP);
    lsthunk_t*   f = lsthunk_get_appl_func(t);
    e->as.app.func = to_expr_from_thunk(f ? f : t); // fallback
    lssize_t argc  = lsthunk_get_argc(t);
    e->as.app.argc = (size_t)argc;
    if (argc > 0) {
      e->as.app.args         = (lstr_expr_t**)calloc((size_t)argc, sizeof(lstr_expr_t*));
      lsthunk_t* const* args = lsthunk_get_args(t);
      for (lssize_t i = 0; i < argc; i++)
        e->as.app.args[i] = to_expr_from_thunk(args[i]);
    }
    return e;
  }
  lstr_expr_t* e = new_expr(LSTR_EXP_VAL);
  e->as.v        = to_val_from_thunk(t);
  return e;
}

const lstr_prog_t* lstr_from_lsti_path(const char* path) {
  lsti_image_t img = { 0 };
  if (lsti_map(path, &img) != 0)
    return NULL;
  if (lsti_validate(&img) != 0) {
    lsti_unmap(&img);
    return NULL;
  }
  lsthunk_t** roots = NULL;
  lssize_t    rootc = 0;
  lstenv_t*   env   = lstenv_new(NULL);
  (void)lsti_materialize(&img, &roots, &rootc, env);
  lstr_prog_t* prog = (lstr_prog_t*)calloc(1, sizeof(lstr_prog_t));
  if (!prog) {
    lsti_unmap(&img);
    return NULL;
  }
  prog->rootc = (size_t)rootc;
  if (rootc > 0) {
    prog->roots = (lstr_expr_t**)calloc((size_t)rootc, sizeof(lstr_expr_t*));
    for (lssize_t i = 0; i < rootc; i++)
      prog->roots[i] = to_expr_from_thunk(roots[i]);
  }
  lsti_unmap(&img);
  return prog;
}

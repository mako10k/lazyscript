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

static lstr_val_t* new_val(lstr_val_kind_t k){ lstr_val_t* v = (lstr_val_t*)calloc(1,sizeof(lstr_val_t)); if(v) v->kind=k; return v; }
static lstr_expr_t* new_expr(lstr_exp_kind_t k){ lstr_expr_t* e = (lstr_expr_t*)calloc(1,sizeof(lstr_expr_t)); if(e) e->kind=k; return e; }

static lstr_val_t* to_val_from_thunk(lsthunk_t* t){
  switch (lsthunk_get_type(t)){
    case LSTTYPE_INT: {
      lstr_val_t* v = new_val(LSTR_VAL_INT);
      const lsint_t* i = lsthunk_get_int(t);
      v->as.i = (long long)lsint_get(i);
      return v;
    }
    case LSTTYPE_STR: {
      lstr_val_t* v = new_val(LSTR_VAL_STR);
      v->as.s = lsthunk_get_str(t);
      return v;
    }
    case LSTTYPE_SYMBOL: {
      lstr_val_t* v = new_val(LSTR_VAL_REF);
      v->as.ref_name = lsthunk_get_symbol(t);
      return v;
    }
    case LSTTYPE_REF: {
      lstr_val_t* v = new_val(LSTR_VAL_REF);
      v->as.ref_name = lsthunk_get_ref_name(t);
      return v;
    }
    case LSTTYPE_ALGE: {
      lstr_val_t* v = new_val(LSTR_VAL_CONSTR);
      v->as.constr.name = lsthunk_get_constr(t);
      lssize_t argc = lsthunk_get_argc(t);
      v->as.constr.argc = (size_t)argc;
      if (argc>0){
        v->as.constr.args = (lstr_val_t**)calloc((size_t)argc, sizeof(lstr_val_t*));
        lsthunk_t* const* args = lsthunk_get_args(t);
        for (lssize_t i=0;i<argc;i++) v->as.constr.args[i] = to_val_from_thunk(args[i]);
      }
      return v;
    }
    case LSTTYPE_CHOICE: {
      lstr_val_t* v = new_val(LSTR_VAL_CHOICE);
      v->as.choice.kind = lsthunk_get_choice_kind(t);
      v->as.choice.left = to_expr_from_thunk(lsthunk_get_choice_left(t));
      v->as.choice.right= to_expr_from_thunk(lsthunk_get_choice_right(t));
      return v;
    }
    case LSTTYPE_BUILTIN: {
      lstr_val_t* v = new_val(LSTR_VAL_REF);
      v->as.ref_name = lsthunk_get_builtin_name(t);
      return v;
    }
    case LSTTYPE_LAMBDA: {
      lstr_val_t* v = new_val(LSTR_VAL_LAM);
      lstpat_t* p = lsthunk_get_param(t);
      // Convert param: REF -> VAR name, otherwise wildcard
      lstr_pat_t* lp = (lstr_pat_t*)calloc(1, sizeof(lstr_pat_t));
      if (lp){
        const lsstr_t* rn = lstpat_get_refname(p);
        if (rn){ lp->kind = LSTR_PAT_VAR; lp->as.var_name = rn; }
        else { lp->kind = LSTR_PAT_WILDCARD; }
      }
      v->as.lam.param = lp;
      v->as.lam.body = to_expr_from_thunk(lsthunk_get_body(t));
      return v;
    }
    case LSTTYPE_BOTTOM: {
      lstr_val_t* v = new_val(LSTR_VAL_BOTTOM);
      v->as.bottom.msg = lsthunk_bottom_get_message(t);
      lssize_t rc = lsthunk_bottom_get_argc(t);
      v->as.bottom.relc = (size_t)rc;
      if (rc>0){
        v->as.bottom.related = (lstr_val_t**)calloc((size_t)rc, sizeof(lstr_val_t*));
        lsthunk_t* const* rel = lsthunk_bottom_get_args(t);
        for (lssize_t i=0;i<rc;i++) v->as.bottom.related[i] = to_val_from_thunk(rel[i]);
      }
      return v;
    }
    default:
      return new_val(LSTR_VAL_BOTTOM); // placeholder for unsupported kinds
  }
}

static lstr_expr_t* to_expr_from_thunk(lsthunk_t* t){
  if (lsthunk_get_type(t) == LSTTYPE_APPL){
    lstr_expr_t* e = new_expr(LSTR_EXP_APP);
    lsthunk_t* f = lsthunk_get_appl_func(t);
    e->as.app.func = to_expr_from_thunk(f ? f : t); // fallback
    lssize_t argc = lsthunk_get_argc(t);
    e->as.app.argc = (size_t)argc;
    if (argc>0){
      e->as.app.args = (lstr_expr_t**)calloc((size_t)argc, sizeof(lstr_expr_t*));
      lsthunk_t* const* args = lsthunk_get_args(t);
      for (lssize_t i=0;i<argc;i++) e->as.app.args[i] = to_expr_from_thunk(args[i]);
    }
    return e;
  }
  lstr_expr_t* e = new_expr(LSTR_EXP_VAL);
  e->as.v = to_val_from_thunk(t);
  return e;
}

const lstr_prog_t* lstr_from_lsti_path(const char* path) {
  lsti_image_t img = {0};
  if (lsti_map(path, &img) != 0) return NULL;
  if (lsti_validate(&img) != 0) { lsti_unmap(&img); return NULL; }
  lsthunk_t** roots = NULL; lssize_t rootc = 0;
  lstenv_t* env = lstenv_new(NULL);
  (void)lsti_materialize(&img, &roots, &rootc, env);
  lstr_prog_t* prog = (lstr_prog_t*)calloc(1, sizeof(lstr_prog_t));
  if (!prog){ lsti_unmap(&img); return NULL; }
  prog->rootc = (size_t)rootc;
  if (rootc>0){
    prog->roots = (lstr_expr_t**)calloc((size_t)rootc, sizeof(lstr_expr_t*));
    for (lssize_t i=0;i<rootc;i++) prog->roots[i] = to_expr_from_thunk(roots[i]);
  }
  lsti_unmap(&img);
  return prog;
}

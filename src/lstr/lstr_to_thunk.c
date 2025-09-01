#include "lstr.h"
#include "lstr_internal.h"
#include "thunk/thunk.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"
#include "common/int.h"
#include "common/str.h"
#include "common/ref.h"
#include "runtime/trace.h"
#include <stdlib.h>

static lsthunk_t* to_thunk_from_expr(const lstr_expr_t* e, lstenv_t* env);

static lstpat_t* to_lstpat_from_pat(const lstr_pat_t* p){
  if (!p) return lstpat_new_wild_raw();
  switch (p->kind){
    case LSTR_PAT_WILDCARD: return lstpat_new_wild_raw();
    case LSTR_PAT_VAR: {
      const lsref_t* r = lsref_new(p->as.var_name, lstrace_take_pending_or_unknown());
      return lstpat_new_ref(r);
    }
    case LSTR_PAT_CONSTR: {
      lstpat_t** subs = NULL;
      if (p->as.constr.argc>0){
        subs = (lstpat_t**)calloc(p->as.constr.argc, sizeof(lstpat_t*));
        for (size_t i=0;i<p->as.constr.argc;i++) subs[i] = to_lstpat_from_pat(p->as.constr.args[i]);
      }
      lstpat_t* q = lstpat_new_alge_raw(p->as.constr.name, (lssize_t)p->as.constr.argc, subs);
      free(subs);
      return q;
    }
    case LSTR_PAT_INT: return lstpat_new_int_raw(lsint_new((int)p->as.i));
    case LSTR_PAT_STR: return lstpat_new_str_raw(p->as.s);
    case LSTR_PAT_AS:  return lstpat_new_as_raw(to_lstpat_from_pat(p->as.as_pat.ref_pat), to_lstpat_from_pat(p->as.as_pat.inner));
    case LSTR_PAT_OR:  return lstpat_new_or_raw(to_lstpat_from_pat(p->as.or_pat.left), to_lstpat_from_pat(p->as.or_pat.right));
    case LSTR_PAT_CARET: return lstpat_new_caret_raw(to_lstpat_from_pat(p->as.caret_inner));
    default: return lstpat_new_wild_raw();
  }
}

static lsthunk_t* to_thunk_from_val(const lstr_val_t* v, lstenv_t* env){
  switch (v->kind){
    case LSTR_VAL_INT:   return lsthunk_new_int(lsint_new((int)v->as.i));
    case LSTR_VAL_STR:   return lsthunk_new_str(v->as.s);
    case LSTR_VAL_REF: {
      // Heuristic: leading '.' indicates symbol literal
      const lsstr_t* nm = v->as.ref_name;
      if (nm && lsstr_get_len(nm)>0 && lsstr_get_buf(nm)[0]=='.') return lsthunk_new_symbol(nm);
      const lsref_t* r = lsref_new(nm, lstrace_take_pending_or_unknown());
      return lsthunk_new_ref(r, env);
    }
    case LSTR_VAL_CONSTR: {
      lsthunk_t* t = lsthunk_alloc_alge(v->as.constr.name, (lssize_t)v->as.constr.argc);
      for (size_t i=0;i<v->as.constr.argc;i++) lsthunk_set_alge_arg(t, (lssize_t)i, to_thunk_from_val(v->as.constr.args[i], env));
      return t;
    }
    case LSTR_VAL_LAM: {
      lstpat_t* param = to_lstpat_from_pat(v->as.lam.param);
      lsthunk_t* t = lsthunk_alloc_lambda(param);
      lsthunk_set_lambda_body(t, to_thunk_from_expr(v->as.lam.body, env));
      return t;
    }
    case LSTR_VAL_BOTTOM: {
      lsthunk_t* t = lsthunk_alloc_bottom(v->as.bottom.msg ? v->as.bottom.msg : "bottom", lstrace_take_pending_or_unknown(), (lssize_t)v->as.bottom.relc);
      for (size_t i=0;i<v->as.bottom.relc;i++) lsthunk_set_bottom_related(t, (lssize_t)i, to_thunk_from_val(v->as.bottom.related[i], env));
      return t;
    }
    case LSTR_VAL_CHOICE: {
      lsthunk_t* t = lsthunk_alloc_choice(v->as.choice.kind);
      lsthunk_set_choice_left(t, to_thunk_from_expr(v->as.choice.left, env));
      lsthunk_set_choice_right(t, to_thunk_from_expr(v->as.choice.right, env));
      return t;
    }
    default: return lsthunk_bottom_here("unsupported LSTR val");
  }
}

static lsthunk_t* to_thunk_from_expr(const lstr_expr_t* e, lstenv_t* env){
  switch (e->kind){
    case LSTR_EXP_VAL: return to_thunk_from_val(e->as.v, env);
    case LSTR_EXP_APP: {
      lsthunk_t* t = lsthunk_alloc_appl((lssize_t)e->as.app.argc);
      lsthunk_set_appl_func(t, to_thunk_from_expr(e->as.app.func, env));
      for (size_t i=0;i<e->as.app.argc;i++) lsthunk_set_appl_arg(t, (lssize_t)i, to_thunk_from_expr(e->as.app.args[i], env));
      return t;
    }
    default: return lsthunk_bottom_here("unsupported LSTR expr");
  }
}

int lstr_materialize_to_thunks(const lstr_prog_t* p, lsthunk_t*** out_roots, lssize_t* out_rootc, lstenv_t* env){
  if (!p || !out_roots || !out_rootc) return -1;
  *out_roots = NULL; *out_rootc = 0;
  if (p->rootc==0) return 0;
  lsthunk_t** roots = (lsthunk_t**)calloc(p->rootc, sizeof(lsthunk_t*));
  if (!roots) return -2;
  for (size_t i=0;i<p->rootc;i++) roots[i] = to_thunk_from_expr(p->roots[i], env);
  *out_roots = roots; *out_rootc = (lssize_t)p->rootc;
  return 0;
}

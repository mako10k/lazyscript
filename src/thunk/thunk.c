#include "thunk/thunk.h"
#include "common/io.h"
#include "common/loc.h"
#include "common/malloc.h"
#include "expr/eclosure.h"
#include "lstypes.h"
#include "thunk/tenv.h"
#include "thunk/tpat.h"
#include "runtime/error.h"
#include "runtime/trace.h"
#include "runtime/effects.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "expr/enslit.h"
#include "expr/eappl.h"
#include "expr/ealge.h"
#include "expr/echoice.h"
// removed duplicate: #include "expr/eclosure.h"
#include "misc/bind.h"
#include "pat/pat.h"
#include "common/array.h"
#include "thunk/thunk_bin.h"
#include <stdint.h>
#include <stdlib.h>

// Debug tracing for thunk evaluation (off by default). Enable with -DLS_TRACE=1
#ifndef LS_TRACE
#define LS_TRACE 0
#endif

// --- Eval-time parameter binding frame (overlay) ---------------------------
typedef struct lseval_bind_ent {
  const lsstr_t*           name;
  lsthunk_t*               thunk;
  struct lseval_bind_ent*  next;
} lseval_bind_ent_t;

typedef struct lseval_bind_frame {
  lseval_bind_ent_t*         head;
  struct lseval_bind_frame*  prev;
} lseval_bind_frame_t;

static lseval_bind_frame_t* g_eval_bind_top = NULL;

static void lseval_bind_push_empty(void) {
  lseval_bind_frame_t* f = lsmalloc(sizeof(lseval_bind_frame_t));
  f->head                = NULL;
  f->prev                = g_eval_bind_top;
  g_eval_bind_top        = f;
}

static void lseval_bind_pop(void) {
  if (g_eval_bind_top)
    g_eval_bind_top = g_eval_bind_top->prev; // nodes are GC-managed
}

static void lseval_bind_add(const lsstr_t* name, lsthunk_t* thunk) {
  if (!g_eval_bind_top)
    lseval_bind_push_empty();
  lseval_bind_ent_t* e = lsmalloc(sizeof(lseval_bind_ent_t));
  e->name              = name;
  e->thunk             = thunk;
  e->next              = g_eval_bind_top->head;
  g_eval_bind_top->head = e;
  const char* dbg = getenv("LAZYSCRIPT_DEBUG_FRAME");
  if (dbg && *dbg) {
    lsprintf(stderr, 0, "[frame] add ");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
    const char* ty = "?";
    if (thunk) {
      switch (lsthunk_get_type(thunk)) {
      case LSTTYPE_INT: ty = "int"; break;
      case LSTTYPE_STR: ty = "str"; break;
      case LSTTYPE_SYMBOL: ty = "symbol"; break;
      case LSTTYPE_ALGE: ty = "alge"; break;
      case LSTTYPE_APPL: ty = "appl"; break;
      case LSTTYPE_LAMBDA: ty = "lambda"; break;
      case LSTTYPE_REF: ty = "ref"; break;
      case LSTTYPE_CHOICE: ty = "choice"; break;
      case LSTTYPE_BUILTIN: ty = "builtin"; break;
      default: ty = "other"; break;
      }
    }
    lsprintf(stderr, 0, " -> %p(%s)\n", (void*)thunk, ty);
  }
}

static lsthunk_t* lseval_bind_lookup(const lsstr_t* name) {
  const char* dbg = getenv("LAZYSCRIPT_DEBUG_FRAME");
  for (lseval_bind_frame_t* f = g_eval_bind_top; f; f = f->prev) {
    for (lseval_bind_ent_t* e = f->head; e; e = e->next) {
      if (lsstrcmp(e->name, name) == 0) {
        if (dbg && *dbg) {
          lsprintf(stderr, 0, "[frame] hit ");
          lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
          lsprintf(stderr, 0, " -> %p\n", (void*)e->thunk);
        }
        return e->thunk;
      }
    }
  }
  if (dbg && *dbg) {
    lsprintf(stderr, 0, "[frame] miss ");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, name);
    lsprintf(stderr, 0, "\n");
  }
  return NULL;
}

static void lseval_bind_collect_from_pat(lstpat_t* pat) {
  switch (lstpat_get_type(pat)) {
  case LSPTYPE_ALGE: {
    lssize_t n = lstpat_get_argc(pat);
    lstpat_t* const* args = lstpat_get_args(pat);
    for (lssize_t i = 0; i < n; i++)
      lseval_bind_collect_from_pat(args[i]);
    break;
  }
  case LSPTYPE_AS:
    lseval_bind_collect_from_pat(lstpat_get_ref(pat));
    lseval_bind_collect_from_pat(lstpat_get_aspattern(pat));
    break;
  case LSPTYPE_REF: {
    const lsstr_t* nm = lstpat_get_refname(pat);
    lsthunk_t*     v  = lstpat_get_refbound(pat);
    if (nm && v)
      lseval_bind_add(nm, v);
    break;
  }
  case LSPTYPE_OR:
    lseval_bind_collect_from_pat(lstpat_get_or_left(pat));
    lseval_bind_collect_from_pat(lstpat_get_or_right(pat));
    break;
  case LSPTYPE_CARET:
    lseval_bind_collect_from_pat((lstpat_t*)lspat_get_caret_inner((const lspat_t*)pat));
    break;
  case LSPTYPE_WILDCARD:
  case LSPTYPE_INT:
  case LSPTYPE_STR:
    break;
  }
}

struct lstalge {
  const lsstr_t* lta_constr;
  lssize_t       lta_argc;
  lsthunk_t*     lta_args[0];
};

struct lstappl {
  lsthunk_t* lta_func;
  lssize_t   lta_argc;
  lsthunk_t* lta_args[0];
};

struct lstchoice {
  lsthunk_t* ltc_left;
  lsthunk_t* ltc_right;
  int        ltc_kind; // 1=lambda-choice ('|'), 2=expr-choice ('||')
};

struct lstbind {
  lstpat_t*  ltb_lhs;
  lsthunk_t* ltb_rhs;
};

struct lstlambda {
  lstpat_t*  ltl_param;
  lsthunk_t* ltl_body;
};

struct lstref_target_origin {
  lstrtype_t lrto_type;
  union {
    lstbind_t   lrto_bind;
    lstlambda_t lrto_lambda;
    lsthunk_t*  lrto_builtin;
  };
};

struct lstref_target {
  lstref_target_origin_t* lrt_origin;
  lstpat_t*               lrt_pat;
};

struct lstref {
  const lsref_t*   ltr_ref;
  lstref_target_t* ltr_target;
  const lstenv_t*  ltr_env;
};

struct lstbuiltin {
  const lsstr_t*    lti_name;
  lssize_t          lti_arity;
  lstbuiltin_func_t lti_func;
  void*             lti_data;
  lsbuiltin_attr_t  lti_attr;
};

typedef struct lsbotrel {
  lssize_t    lbr_argc;
  lsthunk_t** lbr_args; // related thunks (opaque)
} lsbotrel_t;

struct lsthunk {
  lsttype_t  lt_type;
  lsthunk_t* lt_whnf;
  int        lt_trace_id;
  union {
    lstalge_t           lt_alge;
    lstappl_t           lt_appl;
    lstchoice_t         lt_choice;
    lstlambda_t         lt_lambda;
    lstref_t            lt_ref;
    const lsint_t*      lt_int;
    const lsstr_t*      lt_str;
    const lsstr_t*      lt_symbol;
    const lstbuiltin_t* lt_builtin;
    struct {
      const char* lt_msg;
      lsloc_t     lt_loc;
      lsbotrel_t  lt_rel;
    } lt_bottom;
  };
};

static int g_trace_next_id = 0;

// --- Bottom (⊥) -----------------------------------------------------------

lsthunk_t* lsthunk_new_bottom(const char* message, lsloc_t loc, lssize_t argc,
                              lsthunk_t* const* args) {
  lsthunk_t* t   = lsmalloc(sizeof(lsthunk_t));
  t->lt_type     = LSTTYPE_BOTTOM;
  t->lt_whnf     = t; // bottom is WHNF
  t->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(loc.filename ? loc : lstrace_take_pending_or_unknown());
  t->lt_bottom.lt_msg          = message ? message : "";
  t->lt_bottom.lt_loc          = loc;
  t->lt_bottom.lt_rel.lbr_argc = argc;
  if (argc > 0) {
    t->lt_bottom.lt_rel.lbr_args = lsmalloc(sizeof(lsthunk_t*) * argc);
    for (lssize_t i = 0; i < argc; i++)
      t->lt_bottom.lt_rel.lbr_args[i] = args[i];
  } else {
    t->lt_bottom.lt_rel.lbr_args = NULL;
  }
  return t;
}

lsthunk_t* lsthunk_bottom_here(const char* message) {
  return lsthunk_new_bottom(message, lstrace_take_pending_or_unknown(), 0, NULL);
}

int lsthunk_is_bottom(const lsthunk_t* thunk) { return thunk && thunk->lt_type == LSTTYPE_BOTTOM; }
const char* lsthunk_bottom_get_message(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BOTTOM) ? thunk->lt_bottom.lt_msg : NULL;
}
lsloc_t lsthunk_bottom_get_loc(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BOTTOM) ? thunk->lt_bottom.lt_loc
                                                     : lstrace_take_pending_or_unknown();
}
lssize_t lsthunk_bottom_get_argc(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BOTTOM) ? thunk->lt_bottom.lt_rel.lbr_argc : 0;
}
lsthunk_t* const* lsthunk_bottom_get_args(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BOTTOM)
             ? (lsthunk_t* const*)thunk->lt_bottom.lt_rel.lbr_args
             : NULL;
}

static lsloc_t earlier_loc(lsloc_t a, lsloc_t b) {
  // Prefer a if it has a filename and b doesn't, or keep a by default.
  if (a.filename && (!b.filename || strcmp(b.filename, "<unknown>") == 0))
    return a;
  return a.filename ? a : b;
}

lsthunk_t* lsthunk_bottom_merge(lsthunk_t* a, lsthunk_t* b) {
  if (!a)
    return b;
  if (!b)
    return a;
  if (!lsthunk_is_bottom(a))
    return a;
  if (!lsthunk_is_bottom(b))
    return b;
  // Merge messages with "; "
  const char* ma  = lsthunk_bottom_get_message(a);
  const char* mb  = lsthunk_bottom_get_message(b);
  size_t      na  = ma ? strlen(ma) : 0;
  size_t      nb  = mb ? strlen(mb) : 0;
  char*       m   = lsmalloc(na + (na && nb ? 2 : 0) + nb + 1);
  size_t      off = 0;
  if (na) {
    memcpy(m + off, ma, na);
    off += na;
  }
  if (na && nb) {
    m[off++] = ';';
    m[off++] = ' ';
  }
  if (nb) {
    memcpy(m + off, mb, nb);
    off += nb;
  }
  m[off] = '\0';
  // Concatenate args
  lssize_t    ca = lsthunk_bottom_get_argc(a), cb = lsthunk_bottom_get_argc(b);
  lssize_t    cn = ca + cb;
  lsthunk_t** xs = cn ? lsmalloc(sizeof(lsthunk_t*) * cn) : NULL;
  for (lssize_t i = 0; i < ca; i++)
    xs[i] = a->lt_bottom.lt_rel.lbr_args[i];
  for (lssize_t i = 0; i < cb; i++)
    xs[ca + i] = b->lt_bottom.lt_rel.lbr_args[i];
  lsloc_t loc = earlier_loc(a->lt_bottom.lt_loc, b->lt_bottom.lt_loc);
  return lsthunk_new_bottom(m, loc, cn, xs);
}

// --- Internal-friendly constructors for two-phase wiring ---
lsthunk_t* lsthunk_alloc_alge(const lsstr_t* constr, lssize_t argc) {
  lsthunk_t* thunk =
      lsmalloc(lssizeof(lsthunk_t, lt_alge) + (argc > 0 ? (size_t)argc : 0) * sizeof(lsthunk_t*));
  thunk->lt_type     = LSTTYPE_ALGE;
  thunk->lt_whnf     = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_alge.lta_constr = constr;
  thunk->lt_alge.lta_argc   = argc;
  // Leave args uninitialized for the caller to fill via setter
  return thunk;
}

void lsthunk_set_alge_arg(lsthunk_t* thunk, lssize_t idx, lsthunk_t* arg) {
  if (!thunk || thunk->lt_type != LSTTYPE_ALGE)
    return;
  if (idx < 0 || idx >= thunk->lt_alge.lta_argc)
    return;
  thunk->lt_alge.lta_args[idx] = arg;
}

lsthunk_t* lsthunk_alloc_bottom(const char* message, lsloc_t loc, lssize_t argc) {
  lsthunk_t* t   = lsmalloc(sizeof(lsthunk_t));
  t->lt_type     = LSTTYPE_BOTTOM;
  t->lt_whnf     = t; // bottom is WHNF
  t->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(loc.filename ? loc : lstrace_take_pending_or_unknown());
  t->lt_bottom.lt_msg          = message ? message : "";
  t->lt_bottom.lt_loc          = loc;
  t->lt_bottom.lt_rel.lbr_argc = argc;
  if (argc > 0) {
    t->lt_bottom.lt_rel.lbr_args = lsmalloc(sizeof(lsthunk_t*) * (size_t)argc);
    for (lssize_t i = 0; i < argc; i++)
      t->lt_bottom.lt_rel.lbr_args[i] = NULL;
  } else {
    t->lt_bottom.lt_rel.lbr_args = NULL;
  }
  return t;
}

void lsthunk_set_bottom_related(lsthunk_t* thunk, lssize_t idx, lsthunk_t* arg) {
  if (!thunk || thunk->lt_type != LSTTYPE_BOTTOM)
    return;
  if (idx < 0 || idx >= thunk->lt_bottom.lt_rel.lbr_argc)
    return;
  thunk->lt_bottom.lt_rel.lbr_args[idx] = arg;
}

// --- APPL helpers (two-phase wiring) --------------------------------------

lsthunk_t* lsthunk_alloc_appl(lssize_t argc) {
  if (argc < 0)
    return NULL;
  lsthunk_t* t   = lsmalloc(lssizeof(lsthunk_t, lt_appl) + (size_t)argc * sizeof(lsthunk_t*));
  t->lt_type     = LSTTYPE_APPL;
  t->lt_whnf     = NULL;
  t->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  t->lt_appl.lta_func = NULL;
  t->lt_appl.lta_argc = argc;
  for (lssize_t i = 0; i < argc; i++)
    t->lt_appl.lta_args[i] = NULL;
  return t;
}

void lsthunk_set_appl_func(lsthunk_t* thunk, lsthunk_t* func) {
  if (!thunk || thunk->lt_type != LSTTYPE_APPL)
    return;
  thunk->lt_appl.lta_func = func;
}

void lsthunk_set_appl_arg(lsthunk_t* thunk, lssize_t idx, lsthunk_t* arg) {
  if (!thunk || thunk->lt_type != LSTTYPE_APPL)
    return;
  if (idx < 0 || idx >= thunk->lt_appl.lta_argc)
    return;
  thunk->lt_appl.lta_args[idx] = arg;
}

lsthunk_t* lsthunk_get_appl_func(const lsthunk_t* thunk) {
  if (!thunk || thunk->lt_type != LSTTYPE_APPL)
    return NULL;
  return thunk->lt_appl.lta_func;
}

// --- CHOICE helpers -------------------------------------------------------

lsthunk_t* lsthunk_alloc_choice(int kind) {
  lsthunk_t* t   = lsmalloc(lssizeof(lsthunk_t, lt_choice));
  t->lt_type     = LSTTYPE_CHOICE;
  t->lt_whnf     = NULL;
  t->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  t->lt_choice.ltc_left  = NULL;
  t->lt_choice.ltc_right = NULL;
  t->lt_choice.ltc_kind  = kind;
  return t;
}

void lsthunk_set_choice_left(lsthunk_t* thunk, lsthunk_t* left) {
  if (!thunk || thunk->lt_type != LSTTYPE_CHOICE)
    return;
  thunk->lt_choice.ltc_left = left;
}

void lsthunk_set_choice_right(lsthunk_t* thunk, lsthunk_t* right) {
  if (!thunk || thunk->lt_type != LSTTYPE_CHOICE)
    return;
  thunk->lt_choice.ltc_right = right;
}

int lsthunk_get_choice_kind(const lsthunk_t* thunk) {
  if (!thunk || thunk->lt_type != LSTTYPE_CHOICE)
    return 0;
  return thunk->lt_choice.ltc_kind;
}

lsthunk_t* lsthunk_get_choice_left(const lsthunk_t* thunk) {
  if (!thunk || thunk->lt_type != LSTTYPE_CHOICE)
    return NULL;
  return thunk->lt_choice.ltc_left;
}

lsthunk_t* lsthunk_get_choice_right(const lsthunk_t* thunk) {
  if (!thunk || thunk->lt_type != LSTTYPE_CHOICE)
    return NULL;
  return thunk->lt_choice.ltc_right;
}

// --- REF helper -----------------------------------------------------------

const lsstr_t* lsthunk_get_ref_name(const lsthunk_t* thunk) {
  if (!thunk || thunk->lt_type != LSTTYPE_REF)
    return NULL;
  return lsref_get_name(thunk->lt_ref.ltr_ref);
}

// --- LAMBDA helpers (two-phase wiring) -----------------------------------

lsthunk_t* lsthunk_alloc_lambda(lstpat_t* param) {
  lsthunk_t* t   = lsmalloc(sizeof(lsthunk_t));
  t->lt_type     = LSTTYPE_LAMBDA;
  t->lt_whnf     = t;
  t->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  t->lt_lambda.ltl_param = param;
  t->lt_lambda.ltl_body  = NULL;
  return t;
}

void lsthunk_set_lambda_body(lsthunk_t* thunk, lsthunk_t* body) {
  if (!thunk || thunk->lt_type != LSTTYPE_LAMBDA)
    return;
  thunk->lt_lambda.ltl_body = body;
}

lsthunk_t* lsthunk_new_ealge(const lsealge_t* ealge, lstenv_t* tenv) {
  lssize_t               eargc = lsealge_get_argc(ealge);
  const lsexpr_t* const* eargs = lsealge_get_args(ealge);
  lsthunk_t* thunk   = lsmalloc(lssizeof(lsthunk_t, lt_alge) + eargc * sizeof(lsthunk_t*));
  thunk->lt_type     = LSTTYPE_ALGE;
  thunk->lt_whnf     = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_alge.lta_constr = lsealge_get_constr(ealge);
  thunk->lt_alge.lta_argc   = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_alge.lta_args[i] = lsthunk_new_expr(eargs[i], tenv);
  return thunk;
}

lsthunk_t* lsthunk_new_eappl(const lseappl_t* eappl, lstenv_t* tenv) {
  lssize_t               eargc = lseappl_get_argc(eappl);
  const lsexpr_t* const* eargs = lseappl_get_args(eappl);
  lsthunk_t*             func  = lsthunk_new_expr(lseappl_get_func(eappl), tenv);
  if (func == NULL)
    return NULL;
  lsthunk_t* args_buf[eargc];
  for (lssize_t i = 0; i < eargc; i++) {
    args_buf[i] = lsthunk_new_expr(eargs[i], tenv);
    if (args_buf[i] == NULL)
      return NULL;
  }
  lsthunk_t* thunk   = lsmalloc(lssizeof(lsthunk_t, lt_appl) + eargc * sizeof(lsthunk_t*));
  thunk->lt_type     = LSTTYPE_APPL;
  thunk->lt_whnf     = NULL;
  thunk->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_appl.lta_func = func;
  thunk->lt_appl.lta_argc = eargc;
  for (lssize_t i = 0; i < eargc; i++)
    thunk->lt_appl.lta_args[i] = args_buf[i];
  return thunk;
}

lsthunk_t* lsthunk_new_echoice(const lsechoice_t* echoice, lstenv_t* tenv) {
  // Allocate enough space to include the 'choice' union member
  lsthunk_t* thunk   = lsmalloc(lssizeof(lsthunk_t, lt_choice));
  thunk->lt_type     = LSTTYPE_CHOICE;
  thunk->lt_whnf     = NULL;
  thunk->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_choice.ltc_left  = lsthunk_new_expr(lsechoice_get_left(echoice), tenv);
  thunk->lt_choice.ltc_right = lsthunk_new_expr(lsechoice_get_right(echoice), tenv);
  // Persist kind from AST to runtime
  thunk->lt_choice.ltc_kind = (int)lsechoice_get_kind(echoice);
  return thunk;
}

lsthunk_t* lsthunk_new_eclosure(const lseclosure_t* eclosure, lstenv_t* tenv) {
  tenv                           = lstenv_new(tenv);
  lssize_t                ebindc = lseclosure_get_bindc(eclosure);
  const lsbind_t* const*  ebinds = lseclosure_get_binds(eclosure);
  lstref_target_origin_t* origins[ebindc];
  for (lssize_t i = 0; i < ebindc; i++) {
    const lspat_t* lhs            = lsbind_get_lhs(ebinds[i]);
    origins[i]                    = lsmalloc(sizeof(lstref_target_origin_t));
    origins[i]->lrto_type         = LSTRTYPE_BIND;
    origins[i]->lrto_bind.ltb_lhs = lstpat_new_pat(lhs, tenv, origins[i]);
    if (origins[i]->lrto_bind.ltb_lhs == NULL)
      return NULL;
  }
  for (lssize_t i = 0; i < ebindc; i++) {
    const lsexpr_t* rhs           = lsbind_get_rhs(ebinds[i]);
    origins[i]->lrto_bind.ltb_rhs = lsthunk_new_expr(rhs, tenv);
    if (origins[i]->lrto_bind.ltb_rhs == NULL)
      return NULL;
  }
  return lsthunk_new_expr(lseclosure_get_expr(eclosure), tenv);
}

lsthunk_t* lsthunk_new_ref(const lsref_t* ref, lstenv_t* tenv) {
  // Bind name to target identity at expansion time (no evaluation), per layering
  lstref_target_t* target  = tenv ? lstenv_get(tenv, lsref_get_name(ref)) : NULL;
  lsthunk_t*       thunk   = lsmalloc(lssizeof(lsthunk_t, lt_ref));
  thunk->lt_type           = LSTTYPE_REF;
  thunk->lt_whnf           = NULL;
  thunk->lt_trace_id       = g_trace_next_id++;
  thunk->lt_ref.ltr_ref    = ref;
  thunk->lt_ref.ltr_target = target; // may be NULL if not found yet
  thunk->lt_ref.ltr_env    = tenv;
  // Prefer pending loc; fallback to ref's own loc
  {
    lsloc_t loc = lstrace_take_pending_or_unknown();
    if (loc.filename && strcmp(loc.filename, "<unknown>") != 0)
      lstrace_emit_loc(loc);
    else
      lstrace_emit_loc(lsref_get_loc(ref));
  }
  return thunk;
}

lsthunk_t* lsthunk_new_int(const lsint_t* intval) {
  lsthunk_t* thunk   = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type     = LSTTYPE_INT;
  thunk->lt_whnf     = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  thunk->lt_int      = intval;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  return thunk;
}

lsthunk_t* lsthunk_new_str(const lsstr_t* strval) {
  lsthunk_t* thunk   = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type     = LSTTYPE_STR;
  thunk->lt_whnf     = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  thunk->lt_str      = strval;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  return thunk;
}

lsthunk_t* lsthunk_new_symbol(const lsstr_t* sym) {
  lsthunk_t* thunk   = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type     = LSTTYPE_SYMBOL;
  thunk->lt_whnf     = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  thunk->lt_symbol   = sym;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  return thunk;
}

lsthunk_t* lsthunk_new_elambda(const lselambda_t* elambda, lstenv_t* tenv) {
  const lspat_t*  pparam         = lselambda_get_param(elambda);
  const lsexpr_t* ebody          = lselambda_get_body(elambda);
  tenv                           = lstenv_new(tenv);
  lstref_target_origin_t* origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type              = LSTRTYPE_LAMBDA;
  origin->lrto_lambda.ltl_param  = lstpat_new_pat(pparam, tenv, origin);
  if (origin->lrto_lambda.ltl_param == NULL)
    return NULL;
  origin->lrto_lambda.ltl_body = lsthunk_new_expr(ebody, tenv);
  if (origin->lrto_lambda.ltl_body == NULL)
    return NULL;
  lsthunk_t* thunk   = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type     = LSTTYPE_LAMBDA;
  thunk->lt_whnf     = thunk;
  thunk->lt_trace_id = g_trace_next_id++;
  lstrace_emit_loc(lstrace_take_pending_or_unknown());
  thunk->lt_lambda.ltl_param = origin->lrto_lambda.ltl_param;
  thunk->lt_lambda.ltl_body  = origin->lrto_lambda.ltl_body;
  return thunk;
}

lsthunk_t* lsthunk_new_expr(const lsexpr_t* expr, lstenv_t* tenv) {
  // Record pending loc for this expression so JSONL emission uses it
  lstrace_set_pending_loc(lsexpr_get_loc(expr));
  switch (lsexpr_get_type(expr)) {
  case LSETYPE_ALGE:
    return lsthunk_new_ealge(lsexpr_get_alge(expr), tenv);
  case LSETYPE_APPL:
    return lsthunk_new_eappl(lsexpr_get_appl(expr), tenv);
  case LSETYPE_CHOICE:
    return lsthunk_new_echoice(lsexpr_get_choice(expr), tenv);
  case LSETYPE_CLOSURE:
    return lsthunk_new_eclosure(lsexpr_get_closure(expr), tenv);
  case LSETYPE_LAMBDA:
    return lsthunk_new_elambda(lsexpr_get_lambda(expr), tenv);
  case LSETYPE_INT:
    return lsthunk_new_int(lsexpr_get_int(expr));
  case LSETYPE_REF:
    return lsthunk_new_ref(lsexpr_get_ref(expr), tenv);
  case LSETYPE_STR:
    return lsthunk_new_str(lsexpr_get_str(expr));
  case LSETYPE_SYMBOL:
    return lsthunk_new_symbol(lsexpr_get_symbol(expr));
  case LSETYPE_NSLIT: {
    // Core NSLIT: build (key value ...) argv and dispatch to registered evaluator.
    const lsenslit_t* ns   = lsexpr_get_nslit(expr);
    lssize_t          ec   = lsenslit_get_count(ns);
    lssize_t          argc = ec * 2;
    lsthunk_t**       argv = argc ? lsmalloc(sizeof(lsthunk_t*) * argc) : NULL;
    for (lssize_t i = 0; i < ec; i++) {
      const lsstr_t* name = lsenslit_get_name(ns, i);
      argv[2 * i]         = lsthunk_new_symbol(name);
      const lsexpr_t* v   = lsenslit_get_expr(ns, i);
      argv[2 * i + 1]     = lsthunk_new_expr(v, tenv);
    }
    extern lsthunk_t* ls_call_nslit_eval(lssize_t argc, lsthunk_t* const* args);
    lsthunk_t* res = ls_call_nslit_eval(argc, (lsthunk_t* const*)argv);
    if (argv)
      lsfree(argv);
    return res;
  }
  case LSETYPE_RAISE: {
    // Evaluate argument; if Bottom, propagate; else construct Bottom with message and payload.
    const lsexpr_t* aexpr = lsexpr_get_raise_arg(expr);
    lsthunk_t*      aval  = lsthunk_new_expr(aexpr, tenv);
    if (!aval)
      return NULL;
    aval = lsthunk_eval0(aval);
    if (lsthunk_is_bottom(aval))
      return aval;
    // Build message via deep print: "raise: <aval>"
    char*  buf = NULL;
    size_t bl  = 0;
    FILE*  fp  = lsopen_memstream_gc(&buf, &bl);
    lsprintf(fp, 0, "raise: ");
    lsthunk_deep_print(fp, LSPREC_LOWEST, 0, aval);
    fclose(fp);
    lsthunk_t* rel[1] = { aval };
    return lsthunk_new_bottom(buf ? buf : "raise", lstrace_take_pending_or_unknown(), 1, rel);
  }
  }
  assert(0);
}

lsttype_t lsthunk_get_type(const lsthunk_t* thunk) {
  assert(thunk != NULL);
  return thunk->lt_type;
}

const lsstr_t* lsthunk_get_constr(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE);
  return thunk->lt_alge.lta_constr;
}

// removed: lsthunk_get_func (unused)

lssize_t lsthunk_get_argc(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_argc : thunk->lt_appl.lta_argc;
}

lsthunk_t* const* lsthunk_get_args(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_ALGE || thunk->lt_type == LSTTYPE_APPL);
  return thunk->lt_type == LSTTYPE_ALGE ? thunk->lt_alge.lta_args : thunk->lt_appl.lta_args;
}

// removed: lsthunk_get_left (unused)

// removed: lsthunk_get_right (unused)

lstpat_t* lsthunk_get_param(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_param;
}

lsthunk_t* lsthunk_get_body(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  return thunk->lt_lambda.ltl_body;
}

// removed: lsthunk_get_ref_target (unused)

const lsint_t* lsthunk_get_int(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_INT);
  return thunk->lt_int;
}

const lsstr_t* lsthunk_get_str(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_STR);
  return thunk->lt_str;
}

const lsstr_t* lsthunk_get_symbol(const lsthunk_t* thunk) {
  assert(thunk->lt_type == LSTTYPE_SYMBOL);
  return thunk->lt_symbol;
}

lsmres_t lsthunk_match_alge(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_ALGE);
  lsthunk_t* thunk_whnf = lsthunk_eval0(thunk);
  lsttype_t  ttype      = lsthunk_get_type(thunk_whnf);
  if (ttype != LSTTYPE_ALGE)
    return LSMATCH_FAILURE; // TODO: match as list or string
  const lsstr_t* tconstr = lsthunk_get_constr(thunk_whnf);
  const lsstr_t* pconstr = lstpat_get_constr(tpat);
  const char* dbg_match = getenv("LAZYSCRIPT_DEBUG_MATCH");
  if (dbg_match && *dbg_match) {
    lsprintf(stderr, 0, "[match] ALGE p=");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, pconstr);
    lsprintf(stderr, 0, " vs t=");
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, tconstr);
    lsprintf(stderr, 0, " pargc=%ld targc=%ld\n", (long)lstpat_get_argc(tpat), (long)lsthunk_get_argc(thunk_whnf));
  }
  if (lsstrcmp(pconstr, tconstr) != 0)
    return LSMATCH_FAILURE;
  lssize_t pargc = lstpat_get_argc(tpat);
  lssize_t targc = lsthunk_get_argc(thunk_whnf);
  if (pargc != targc)
    return LSMATCH_FAILURE;
  lstpat_t* const*  pargs = lstpat_get_args(tpat);
  lsthunk_t* const* targs = lsthunk_get_args(thunk_whnf);
  for (lssize_t i = 0; i < pargc; i++)
    if (lsthunk_match_pat(targs[i], pargs[i]) < 0)
      return LSMATCH_FAILURE;
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pas(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_AS);
  lstpat_t* tpref       = lstpat_get_ref(tpat);
  lstpat_t* tpaspattern = lstpat_get_aspattern(tpat);
  lsmres_t  mres        = lsthunk_match_pat(thunk, tpaspattern);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  mres = lsthunk_match_ref(thunk, tpref);
  if (mres != LSMATCH_SUCCESS)
    return mres;
  return LSMATCH_SUCCESS;
}

/**
 * Match an integer value with a thunk
 * @param thunk The thunk
 * @param tpat The integer value
 * @return The result
 */
static lsmres_t lsthunk_match_int(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_INT);
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_INT)
    return LSMATCH_FAILURE;
  const lsint_t* pintval = lstpat_get_int(tpat);
  const lsint_t* tintval = lsthunk_get_int(thunk);
  return lsint_eq(pintval, tintval) ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

/**
 * Match a string value with a thunk
 * @param thunk The thunk
 * @param tpat The string value
 * @return The result
 */
static lsmres_t lsthunk_match_str(lsthunk_t* thunk, const lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_STR);
  lsttype_t ttype = lsthunk_get_type(thunk);
  if (ttype != LSTTYPE_STR)
    return LSMATCH_FAILURE; // TODO: match as list
  const lsstr_t* pstrval = lstpat_get_str(tpat);
  const lsstr_t* tstrval = lsthunk_get_str(thunk);
  return lsstrcmp(pstrval, tstrval) == 0 ? LSMATCH_SUCCESS : LSMATCH_FAILURE;
}

lsmres_t lsthunk_match_ref(lsthunk_t* thunk, lstpat_t* tpat) {
  assert(lstpat_get_type(tpat) == LSPTYPE_REF);
  // Bottom does not bind to variables
  if (lsthunk_is_bottom(thunk))
    return LSMATCH_FAILURE;
  // Bind to WHNF to avoid capturing indirect refs (e.g., zs bound to xs ref)
  lsthunk_t* val = lsthunk_eval0(thunk);
  // Record binding into pattern (legacy) and into eval-time overlay frame (by name)
  lstpat_set_refbound(tpat, val);
  const lsstr_t* nm = lstpat_get_refname(tpat);
  if (nm)
    lseval_bind_add(nm, val);
  return LSMATCH_SUCCESS;
}

lsmres_t lsthunk_match_pat(lsthunk_t* thunk, lstpat_t* tpat) {
  switch (lstpat_get_type(tpat)) {
  case LSPTYPE_ALGE:
    return lsthunk_match_alge(thunk, tpat);
  case LSPTYPE_AS:
    return lsthunk_match_pas(thunk, tpat);
  case LSPTYPE_INT:
    return lsthunk_match_int(thunk, tpat);
  case LSPTYPE_STR:
    return lsthunk_match_str(thunk, tpat);
  case LSPTYPE_REF:
    return lsthunk_match_ref(thunk, tpat);
  case LSPTYPE_WILDCARD:
    // Bottom does not match wildcard per spec
    return lsthunk_is_bottom(thunk) ? LSMATCH_FAILURE : LSMATCH_SUCCESS;
  case LSPTYPE_CARET: {
    // Force WHNF to observe Bottom produced by expressions like (~prelude .raise) x
    lsthunk_t* thunk_whnf = lsthunk_eval0(thunk);
    if (!lsthunk_is_bottom(thunk_whnf))
      return LSMATCH_FAILURE;
    // TODO: consider matching additional info (message/args) in future
    lstpat_t* inner = lstpat_get_caret_inner(tpat);
    // For caret pattern: only Bottom matches. Semantics:
    //  - ^(_)           : succeed (ignore payload)
    //  - ^(~x)         : bind ~x to Bottom's related payload (first related arg). If none, fail.
    //  - other patterns: currently unsupported (fail). TODO: allow matching message/args in the
    //  future.
    if (lstpat_get_type(inner) == LSPTYPE_WILDCARD)
      return LSMATCH_SUCCESS;
    if (lstpat_get_type(inner) == LSPTYPE_REF) {
      lssize_t          argc = lsthunk_bottom_get_argc(thunk_whnf);
      lsthunk_t* const* args = lsthunk_bottom_get_args(thunk_whnf);
      if (argc <= 0 || args == NULL)
        return LSMATCH_FAILURE;
      lstpat_set_refbound(inner, args[0]);
      return LSMATCH_SUCCESS;
    }
    // Other inners are unsupported for now
    return LSMATCH_FAILURE;
  }
  case LSPTYPE_OR: {
    // Try left; on failure, clear any ref bindings and try right
    lstpat_t* left  = lstpat_get_or_left(tpat);
    lstpat_t* right = lstpat_get_or_right(tpat);
    lsmres_t  mres  = lsthunk_match_pat(thunk, left);
    if (mres == LSMATCH_SUCCESS)
      return LSMATCH_SUCCESS;
    lstpat_clear_binds(left);
    return lsthunk_match_pat(thunk, right);
  }
  }
  return LSMATCH_FAILURE;
}

static lsthunk_t* lsthunk_eval_alge(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (C a b ...) x y ... = C a b ... x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_ALGE);
  if (argc == 0)
    return thunk; // it already is in WHNF
  lssize_t targc = lsthunk_get_argc(thunk);
  // Allocate enough space for existing args + new args
  lsthunk_t* thunk_new =
      lsmalloc(lssizeof(lsthunk_t, lt_alge) + (targc + argc) * sizeof(lsthunk_t*));
  thunk_new->lt_type = LSTTYPE_ALGE;
  // This node is already in WHNF; point to self, not the old thunk
  thunk_new->lt_whnf            = thunk_new;
  thunk_new->lt_trace_id        = thunk->lt_trace_id;
  thunk_new->lt_alge.lta_constr = thunk->lt_alge.lta_constr;
  thunk_new->lt_alge.lta_argc   = targc + argc;
  for (lssize_t i = 0; i < targc; i++)
    thunk_new->lt_alge.lta_args[i] = thunk->lt_alge.lta_args[i];
  for (lssize_t i = 0; i < argc; i++)
    thunk_new->lt_alge.lta_args[targc + i] = args[i];
  return thunk_new;
}

static lsthunk_t* lsthunk_eval_appl(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (f a b ...) x y ... => eval f a b ... x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_APPL);
  if (argc == 0)
    return lsthunk_eval0(thunk);
  lsthunk_t*        func1 = thunk->lt_appl.lta_func;
  lssize_t          argc1 = thunk->lt_appl.lta_argc + argc;
  lsthunk_t* const* args1 = (lsthunk_t* const*)lsa_concata(
      thunk->lt_appl.lta_argc, (const void* const*)thunk->lt_appl.lta_args, argc,
      (const void* const*)args);
  return lsthunk_eval(func1, argc1, args1);
}

static lsthunk_t* lsthunk_eval_lambda(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (\param -> body) x y ... = eval (body[param := x]) y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_LAMBDA);
  assert(argc > 0);
  assert(args != NULL);
#if LS_TRACE
  lsprintf(stderr, 0, "DBG lambda: apply argc=%ld\n", (long)argc);
#endif
  lstpat_t*         param = lsthunk_get_param(thunk);
  lsthunk_t*        body  = lsthunk_get_body(thunk);
  lsthunk_t*        arg;
  lsthunk_t* const* args1 =
      (lsthunk_t* const*)lsa_shift(argc, (const void* const*)args, (const void**)&arg);
  if (arg) {
#if LS_TRACE
    const char* at = "?";
    switch (lsthunk_get_type(arg)) {
    case LSTTYPE_INT:
      at = "int";
      break;
    case LSTTYPE_STR:
      at = "str";
      break;
    case LSTTYPE_ALGE:
      at = "alge";
      break;
    case LSTTYPE_APPL:
      at = "appl";
      break;
    case LSTTYPE_LAMBDA:
      at = "lambda";
      break;
    case LSTTYPE_REF:
      at = "ref";
      break;
    case LSTTYPE_CHOICE:
      at = "choice";
      break;
    case LSTTYPE_BUILTIN:
      at = "builtin";
      break;
    }
    lsprintf(stderr, 0, "DBG lambda: arg type=%s\n", at);
#endif
  }
  // Prepare an eval-time frame BEFORE matching so any temporary/ref binds during
  // matching stay scoped to this application and don't leak into callers.
  lseval_bind_push_empty();
  // Match and then expose parameter bindings via the current eval-time frame
  // (not via mutable pattern state)
  body          = lsthunk_clone(body);
  lsmres_t mres = lsthunk_match_pat(arg, param);
  if (mres != LSMATCH_SUCCESS) {
#if LS_TRACE
    lsprintf(stderr, 0, "DBG lambda: match failed\n");
#endif
  // Pattern mismatch -> bottom (no match), carry arg as related context
  // Clear bindings established during a failed match attempt to avoid leaking into siblings
    lstpat_clear_binds(param);
  // Pop the eval-time frame created for this application
  lseval_bind_pop();
    lsthunk_t* rels[1] = { arg };
    return lsthunk_new_bottom("lambda match failure", lstrace_take_pending_or_unknown(), 1, rels);
  }
#if LS_TRACE
  lsprintf(stderr, 0, "DBG lambda: eval body\n");
#endif
  // Evaluate lambda body within the same frame containing parameter binds
  lsthunk_t* ret = lsthunk_eval(body, argc - 1, args1);
  if (ret == NULL) {
#if LS_TRACE
    lsprintf(stderr, 0, "DBG lambda: body eval returned NULL\n");
#endif
    // Clear parameter bindings before early return
    lstpat_clear_binds(param);
    // Pop frame on early return
    lseval_bind_pop();
    return NULL;
  }
  // Capture, then pop eval-time frame. Clear pattern binds to avoid cross-apply leakage.
  ret = lsthunk_subst_param(ret, param);
  lseval_bind_pop();
  lstpat_clear_binds(param);
  if (ret) {
#if LS_TRACE
    const char* rt = "?";
    switch (lsthunk_get_type(ret)) {
    case LSTTYPE_INT:
      rt = "int";
      break;
    case LSTTYPE_STR:
      rt = "str";
      break;
    case LSTTYPE_ALGE:
      rt = "alge";
      break;
    case LSTTYPE_APPL:
      rt = "appl";
      break;
    case LSTTYPE_LAMBDA:
      rt = "lambda";
      break;
    case LSTTYPE_REF:
      rt = "ref";
      break;
    case LSTTYPE_CHOICE:
      rt = "choice";
      break;
    case LSTTYPE_BUILTIN:
      rt = "builtin";
      break;
    }
    lsprintf(stderr, 0, "DBG lambda: ret type=%s\n", rt);
#endif
  }
  return ret;
}

static lsthunk_t* lsthunk_eval_builtin(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_BUILTIN);
#if LS_TRACE
  const lsstr_t* bname = lsthunk_get_builtin_name(thunk);
  lsprintf(stderr, 0, "DBG builtin: call ");
  if (bname)
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, bname);
  else
    lsprintf(stderr, 0, "<anon>");
  lsprintf(stderr, 0, " argc=%ld (arity=%ld)\n", (long)argc, (long)thunk->lt_builtin->lti_arity);
#endif
  if (argc == 0) {
    // For zero-arity builtins, invoke immediately to obtain the value.
    if (thunk->lt_builtin->lti_arity == 0) {
      lstbuiltin_func_t f0   = thunk->lt_builtin->lti_func;
      void*             d0   = thunk->lt_builtin->lti_data;
      lsthunk_t*        ret0 = f0(0, NULL, d0);
      return ret0 ? lsthunk_eval0(ret0) : ls_make_err("builtin: null");
    }
    // Otherwise, it's a function waiting for more args.
    return thunk;
  }
  lssize_t          arity = thunk->lt_builtin->lti_arity;
  lstbuiltin_func_t func  = thunk->lt_builtin->lti_func;
  void*             data  = thunk->lt_builtin->lti_data;
  lsbuiltin_attr_t  attr  = thunk->lt_builtin->lti_attr;
  // Central attribute guards
  if ((attr & LSBATTR_EFFECT) && !ls_effects_allowed()) {
    lsprintf(stderr, 0, "E: builtin: effects not allowed\n");
    return NULL;
  }
  if ((attr & (LSBATTR_ENV_READ | LSBATTR_ENV_WRITE)) && data == NULL) {
    return ls_make_err("builtin: missing env");
  }
  if (argc < arity) {
    lsthunk_t* ret        = lsmalloc(lssizeof(lsthunk_t, lt_appl) + argc * sizeof(lsthunk_t*));
    ret->lt_type          = LSTTYPE_APPL;
    ret->lt_whnf          = ret;
    ret->lt_appl.lta_func = thunk;
    ret->lt_appl.lta_argc = argc;
    for (lssize_t i = 0; i < argc; i++)
      ret->lt_appl.lta_args[i] = args[i];
    return ret;
  }
  lsthunk_t* ret = func(arity, args, data);
#if LS_TRACE
  lsprintf(stderr, 0, "DBG builtin: ");
  if (bname)
    lsstr_print_bare(stderr, LSPREC_LOWEST, 0, bname);
  else
    lsprintf(stderr, 0, "<anon>");
  lsprintf(stderr, 0, " returned %s\n", ret ? "ok" : "NULL");
#endif
  if (ret == NULL)
    return ls_make_err("builtin: null");
  if (lsthunk_is_err(ret))
    return ret;
  return lsthunk_eval(ret, argc - arity, args + arity);
}

static lsthunk_t* lsthunk_eval_ref(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval ~r x y ... = eval (eval ~r) x y ...
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_REF);
  assert(argc == 0 || args != NULL);
#if LS_TRACE
  lsprintf(stderr, 0, "DBG ref: begin\n");
#endif
  // First, check eval-time binding frame for a direct value
  const lsstr_t* rname = lsref_get_name(thunk->lt_ref.ltr_ref);
  lsthunk_t*     bval  = lseval_bind_lookup(rname);
  if (bval != NULL)
    return lsthunk_eval(bval, argc, args);
  // Then resolve from (captured) environment
  lstref_target_t* target = lstenv_get(thunk->lt_ref.ltr_env, rname);
  if (target == NULL) {
      lsprintf(stderr, 0, "E: ");
      lsloc_print(stderr, lsref_get_loc(thunk->lt_ref.ltr_ref));
      lsprintf(stderr, 0, "undefined reference: ");
      lsref_print(stderr, LSPREC_LOWEST, 0, thunk->lt_ref.ltr_ref);
      lsprintf(stderr, 0, "\n");
      // Optional eager trace stack print while stack is still active (test hook)
      const char* eager = getenv("LAZYSCRIPT_TRACE_EAGER_PRINT");
      if (eager && *eager && g_lstrace_table) {
        int         depth = 1;
        const char* d     = getenv("LAZYSCRIPT_TRACE_STACK_DEPTH");
        if (d && *d) {
          int v = atoi(d);
          if (v >= 0)
            depth = v;
        }
        lstrace_print_stack(stderr, depth);
        // lstrace_print_stack starts each frame with "\n at ", so no extra newline needed
      }
      return ls_make_err("undefined reference");
  }
  lstref_target_origin_t* origin = target->lrt_origin;
  assert(origin != NULL);
  lstpat_t* pat_ref = target->lrt_pat;
  assert(pat_ref != NULL);
  lsthunk_t* refbound = lstpat_get_refbound(pat_ref);
  if (refbound != NULL)
    return lsthunk_eval(refbound, argc, args);
  switch (origin->lrto_type) {
  case LSTRTYPE_BIND: {
    lsmres_t mres = lsthunk_match_pat(origin->lrto_bind.ltb_rhs, origin->lrto_bind.ltb_lhs);
    if (mres != LSMATCH_SUCCESS)
      return ls_make_err("ref match failure");
    refbound = lstpat_get_refbound(pat_ref);
    assert(refbound != NULL);
    return lsthunk_eval(refbound, argc, args);
  }
  case LSTRTYPE_LAMBDA: {
    // Parameter refs should have been bound during lambda application.
    refbound = lstpat_get_refbound(pat_ref);
    if (refbound == NULL) {
      // Check eval-time overlay frame by ref-name as a safety-net
      const lsstr_t* nm = lstpat_get_refname(pat_ref);
      lsthunk_t*     fv = nm ? lseval_bind_lookup(nm) : NULL;
      if (fv)
        return lsthunk_eval(fv, argc, args);
      lsprintf(stderr, 0, "E: unbound lambda parameter reference\n");
      // Additionally, print the definition location of the unbound variable (from the pattern)
      const lsref_t* defref = lstpat_get_refref(pat_ref);
      if (defref) {
        lsprintf(stderr, 0, "  defined at ");
        lsloc_print(stderr, lsref_get_loc(defref));
        lsprintf(stderr, 0, " ");
        lsref_print(stderr, LSPREC_LOWEST, 0, defref);
        lsprintf(stderr, 0, "\n");
      }
      // Optional extra debug (env-gated) to help diagnose binding lifecycle issues
      const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
      if (dbg && *dbg) {
        lsprintf(stderr, 0, "  [dbg] origin.param=%p, pat_ref=%p, target=%p\n",
                 (void*)origin->lrto_lambda.ltl_param, (void*)pat_ref, (void*)target);
      }
      return ls_make_err("unbound lambda param");
    }
    return lsthunk_eval(refbound, argc, args);
  }
  case LSTRTYPE_BUILTIN:
    return lsthunk_eval_builtin(origin->lrto_builtin, argc, args);
  default:
    lsprintf(stderr, 0, "E: ref origin: unknown type %d\n", (int)origin->lrto_type);
    return ls_make_err("ref origin type");
  }
}

static int is_lambda_match_failure_err(lsthunk_t* err) {
  if (!lsthunk_is_bottom(err))
    return 0;
  const char* m = lsthunk_bottom_get_message(err);
  return m && strcmp(m, "lambda match failure") == 0;
}

static lsthunk_t* lsthunk_eval_choice(lsthunk_t* thunk, lssize_t argc, lsthunk_t* const* args) {
  // eval (l | r) x y ...
  // For lambda-choice ('|'):
  //   Guard only on the FIRST parameter:
  //   = if argc > 0 then
  //       let v1 = eval l x in
  //         if v1 is "lambda match failure" then eval r x y ... else eval v1 y ...
  //     else
  //       eval l (no fallback occurs at this point)
  //   This ensures deeper (second or later) parameter match failures result in Bottom without
  //   evaluating the right arm, matching the sugar: \P1 P2 -> E1  |  \Q1 Q2 -> E2
  //   where only P1 vs Q1 controls the choice.
  // For expr-choice ('||'):
  //   = let v = eval l x y ... in if v is Bottom then eval r x y ... else v
  assert(thunk != NULL);
  assert(thunk->lt_type == LSTTYPE_CHOICE);
  assert(argc == 0 || args != NULL);
  // Lambda-choice: special stepwise handling for first-arg guard
  if (thunk->lt_choice.ltc_kind == 1 /* LSECHOICE_LAMBDA */ && argc > 0) {
    const char* dbg_choice = getenv("LAZYSCRIPT_DEBUG_CHOICE");
  if (dbg_choice && *dbg_choice) {
      lsprintf(stderr, 0, "[choice] guard arg0 type=");
      if (!args[0]) {
        lsprintf(stderr, 0, "<null>\n");
      } else {
        switch (lsthunk_get_type(args[0])) {
        case LSTTYPE_INT: lsprintf(stderr, 0, "int\n"); break;
        case LSTTYPE_STR: lsprintf(stderr, 0, "str\n"); break;
        case LSTTYPE_SYMBOL: lsprintf(stderr, 0, "symbol\n"); break;
        case LSTTYPE_ALGE: {
          lsprintf(stderr, 0, "alge constr=");
          lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsthunk_get_constr(args[0]));
          lsprintf(stderr, 0, " argc=%ld\n", (long)lsthunk_get_argc(args[0]));
          break; }
        case LSTTYPE_APPL: lsprintf(stderr, 0, "appl\n"); break;
        case LSTTYPE_LAMBDA: lsprintf(stderr, 0, "lambda\n"); break;
        case LSTTYPE_REF: lsprintf(stderr, 0, "ref name=");
          if (args[0]->lt_ref.ltr_ref)
            lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(args[0]->lt_ref.ltr_ref));
          lsprintf(stderr, 0, "\n");
          break;
        case LSTTYPE_CHOICE: lsprintf(stderr, 0, "choice\n"); break;
        case LSTTYPE_BUILTIN: lsprintf(stderr, 0, "builtin\n"); break;
        default: lsprintf(stderr, 0, "other\n"); break;
        }
        // Also inspect WHNF of arg0 for clarity
        lsthunk_t* w0 = lsthunk_eval0(args[0]);
        lsprintf(stderr, 0, "[choice] guard arg0 whnf=");
        if (!w0) {
          lsprintf(stderr, 0, "<null>\n");
        } else {
          switch (lsthunk_get_type(w0)) {
          case LSTTYPE_ALGE: {
            lsprintf(stderr, 0, "alge constr=");
            lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsthunk_get_constr(w0));
            lsprintf(stderr, 0, " argc=%ld\n", (long)lsthunk_get_argc(w0));
            break; }
          case LSTTYPE_REF: {
            lsprintf(stderr, 0, "ref name=");
            if (w0->lt_ref.ltr_ref)
              lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(w0->lt_ref.ltr_ref));
            lsprintf(stderr, 0, "\n");
            break; }
          case LSTTYPE_INT: lsprintf(stderr, 0, "int\n"); break;
          case LSTTYPE_STR: lsprintf(stderr, 0, "str\n"); break;
          case LSTTYPE_APPL: lsprintf(stderr, 0, "appl\n"); break;
          case LSTTYPE_LAMBDA: lsprintf(stderr, 0, "lambda\n"); break;
          case LSTTYPE_CHOICE: lsprintf(stderr, 0, "choice\n"); break;
          case LSTTYPE_BUILTIN: lsprintf(stderr, 0, "builtin\n"); break;
          default: lsprintf(stderr, 0, "other\n"); break;
          }
        }
      }
    }
    // Apply only the first argument to the left arm
    lsthunk_t* left1 = lsthunk_eval(thunk->lt_choice.ltc_left, 1, &args[0]);
    if (dbg_choice && *dbg_choice) {
      const char* ty = "other";
      if (!left1)
        ty = "NULL";
      else if (lsthunk_is_bottom(left1))
        ty = "<bottom>";
      else {
        switch (lsthunk_get_type(left1)) {
        case LSTTYPE_LAMBDA: ty = "lambda"; break;
        case LSTTYPE_ALGE: ty = "alge"; break;
        case LSTTYPE_APPL: ty = "appl"; break;
        case LSTTYPE_REF: ty = "ref"; break;
        default: break;
        }
      }
      lsprintf(stderr, 0, "[choice] apply-left argc=%ld -> %s\n", (long)argc, ty);
    }
    if (left1 == NULL) {
      // Treat as left failure and try right with full args
      if (dbg_choice && *dbg_choice) {
        lsprintf(stderr, 0, "[choice] left -> NULL, try-right argc=%ld\n", (long)argc);
      }
      return lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
    }
    if (lsthunk_is_err(left1)) {
      // Fallback ONLY when the first-parameter match failed
      if (is_lambda_match_failure_err(left1)) {
        if (dbg_choice && *dbg_choice) {
          lsprintf(stderr, 0, "[choice] left -> lambda-match-failure, fallback-right\n");
        }
        lsthunk_t* right = lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
        if (right == NULL)
          return right;
        if (lsthunk_is_bottom(right)) {
          return lsthunk_bottom_merge(left1, right);
        }
        return right;
      }
      // Other bottoms (if any) commit to left; do not fallback
      if (dbg_choice && *dbg_choice) {
        lsprintf(stderr, 0, "[choice] left -> bottom(non-match), commit-left\n");
      }
      return left1;
    }
    // Commit to left: apply remaining args without any further fallback
    if (dbg_choice && *dbg_choice) {
      lsprintf(stderr, 0, "[choice] left -> success, apply rest argc=%ld\n", (long)(argc - 1));
    }
    return lsthunk_eval(left1, argc - 1, args + 1);
  }
  // Catch-choice: left '^|' rightLamChain
  //   Evaluate left with args; if result is Bottom, apply right to the Bottom as a single argument.
  if (thunk->lt_choice.ltc_kind == 3 /* LSECHOICE_CATCH */) {
    lsthunk_t* left = lsthunk_eval(thunk->lt_choice.ltc_left, argc, args);
    if (left == NULL) {
      // Treat as error; cannot catch NULL, so propagate via right applied to a bottom value
      lsthunk_t* b        = lsthunk_bottom_here("null");
      lsthunk_t* rargs[1] = { b };
      return lsthunk_eval(thunk->lt_choice.ltc_right, 1, rargs);
    }
    if (lsthunk_is_bottom(left)) {
      // Apply right lamchain to bottom payload
      lsthunk_t* rargs[1] = { left };
      return lsthunk_eval(thunk->lt_choice.ltc_right, 1, rargs);
    }
    // Non-bottom: pass through
    return left;
  }
  // Default behavior: evaluate left fully, then decide fallback
  lsthunk_t* left = lsthunk_eval(thunk->lt_choice.ltc_left, argc, args);
  if (left == NULL)
    return lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
  if (lsthunk_is_err(left)) {
    int is_lambda_choice = (thunk->lt_choice.ltc_kind == 1);
    int fallback = is_lambda_choice ? is_lambda_match_failure_err(left) : lsthunk_is_bottom(left);
    if (fallback) {
      lsthunk_t* right = lsthunk_eval(thunk->lt_choice.ltc_right, argc, args);
      if (right == NULL)
        return right;
      if (lsthunk_is_bottom(right)) {
        // Accumulate info from both bottoms
        return lsthunk_bottom_merge(left, right);
      }
      // Right succeeded: discard left bottom
      return right;
    }
    // Non-bottom errors should not exist anymore; but if they do, propagate
    return left;
  }
  // Success on left: commit left-biased result.
  return left;
}

lsthunk_t* lsthunk_eval(lsthunk_t* func, lssize_t argc, lsthunk_t* const* args) {
  assert(func != NULL);
  // Enter trace context for this evaluation
  if (func->lt_trace_id >= 0)
    lstrace_push(func->lt_trace_id);
  if (lsthunk_is_err(func)) {
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return func;
  }
  if (argc == 0) {
    lsthunk_t* r = lsthunk_eval0(func);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  switch (lsthunk_get_type(func)) {
  case LSTTYPE_ALGE: {
    lsthunk_t* r = lsthunk_eval_alge(func, argc, args);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  case LSTTYPE_APPL: {
    lsthunk_t* r = lsthunk_eval_appl(func, argc, args);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  case LSTTYPE_LAMBDA: {
    lsthunk_t* r = lsthunk_eval_lambda(func, argc, args);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  case LSTTYPE_REF: {
    lsthunk_t* r = lsthunk_eval_ref(func, argc, args);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  case LSTTYPE_CHOICE: {
    lsthunk_t* r = lsthunk_eval_choice(func, argc, args);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  case LSTTYPE_INT:
    lsprintf(stderr, 0, "F: cannot apply for integer\n");
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return NULL;
  case LSTTYPE_STR:
    lsprintf(stderr, 0, "F: cannot apply for string\n");
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return NULL;
  case LSTTYPE_SYMBOL:
    lsprintf(stderr, 0, "F: cannot apply for symbol\n");
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return NULL;
  case LSTTYPE_BOTTOM:
    // Bottom applied is still bottom
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return func;
  case LSTTYPE_BUILTIN: {
    lsthunk_t* r = lsthunk_eval_builtin(func, argc, args);
    if (func->lt_trace_id >= 0)
      lstrace_pop();
    return r;
  }
  }
  if (func->lt_trace_id >= 0)
    lstrace_pop();
  return NULL;
}

lsthunk_t* lsthunk_eval0(lsthunk_t* thunk) {
  assert(thunk != NULL);
  if (thunk->lt_whnf != NULL)
    return thunk->lt_whnf;
  if (thunk->lt_trace_id >= 0)
    lstrace_push(thunk->lt_trace_id);
  switch (thunk->lt_type) {
  case LSTTYPE_APPL:
    thunk->lt_whnf =
        lsthunk_eval(thunk->lt_appl.lta_func, thunk->lt_appl.lta_argc, thunk->lt_appl.lta_args);
    break;
  case LSTTYPE_REF:
    thunk->lt_whnf = lsthunk_eval_ref(thunk, 0, NULL);
    break;
  case LSTTYPE_CHOICE:
    thunk->lt_whnf = lsthunk_eval_choice(thunk, 0, NULL);
    break;
  case LSTTYPE_BOTTOM:
    thunk->lt_whnf = thunk;
    break;
  case LSTTYPE_BUILTIN:
    thunk->lt_whnf = lsthunk_eval_builtin(thunk, 0, NULL);
    break;
  default:
    // it already is in WHNF
    thunk->lt_whnf = thunk;
    break;
  }
  if (thunk->lt_trace_id >= 0)
    lstrace_pop();
  return thunk->lt_whnf;
}

lstref_target_t* lstref_target_new(lstref_target_origin_t* origin, lstpat_t* pat) {
  lstref_target_t* target = lsmalloc(sizeof(lstref_target_t));
  target->lrt_origin      = origin;
  target->lrt_pat         = pat;
  return target;
}

lstpat_t*  lstref_target_get_pat(lstref_target_t* target) { return target->lrt_pat; }

lsthunk_t* lsthunk_new_builtin(const lsstr_t* name, lssize_t arity, lstbuiltin_func_t func,
                               void* data) {
  lsthunk_t* thunk = lsmalloc(sizeof(lsthunk_t));
  thunk->lt_type   = LSTTYPE_BUILTIN;
  // Do not mark builtins as WHNF at construction. This allows eval0 to
  // execute zero-arity builtins and cache their resulting value, keeping
  // wrappers (e.g., namespace member getters) transparent when printing.
  thunk->lt_whnf        = NULL;
  thunk->lt_trace_id    = -1;
  lstbuiltin_t* builtin = lsmalloc(sizeof(lstbuiltin_t));
  builtin->lti_name     = name;
  builtin->lti_arity    = arity;
  builtin->lti_func     = func;
  builtin->lti_data     = data;
  builtin->lti_attr     = LSBATTR_PURE;
  thunk->lt_builtin     = builtin;
  return thunk;
}

lsthunk_t* lsthunk_new_builtin_attr(const lsstr_t* name, lssize_t arity, lstbuiltin_func_t func,
                                    void* data, lsbuiltin_attr_t attr) {
  lsthunk_t* t = lsthunk_new_builtin(name, arity, func, data);
  if (t && t->lt_type == LSTTYPE_BUILTIN) {
    ((lstbuiltin_t*)t->lt_builtin)->lti_attr = attr;
  }
  return t;
}

lstref_target_origin_t* lstref_target_origin_new_builtin(const lsstr_t* name, lssize_t arity,
                                                         lstbuiltin_func_t func, void* data) {
  lstref_target_origin_t* origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type              = LSTRTYPE_BUILTIN;
  origin->lrto_builtin           = lsthunk_new_builtin(name, arity, func, data);
  return origin;
}

// Helper: create a value-binding target for a concrete value 'value' under symbol 'name'.
// It mirrors closure bind targets but for an already-evaluated RHS thunk.
lstref_target_t* lstref_target_new_value_binding(const lsstr_t* name, lsthunk_t* value) {
  if (!name || !value)
    return NULL;
  // Build a synthetic BIND origin with lhs as REF pattern to 'name' and rhs as the value thunk.
  lstref_target_origin_t* origin = lsmalloc(sizeof(lstref_target_origin_t));
  origin->lrto_type              = LSTRTYPE_BIND;
  // Create lhs pattern and wire into a target so it can be found in env
  lsloc_t   loc  = lsloc("<value>", 1, 1, 1, 1);
  const lsref_t* r    = lsref_new(name, loc);
  lstpat_t*      lhs  = lstpat_new_ref(r);
  origin->lrto_bind.ltb_lhs = lhs;
  origin->lrto_bind.ltb_rhs = value;
  // Create target wrapping this origin and lhs pattern
  lstref_target_t* target = lstref_target_new(origin, lhs);
  return target;
}

int lsthunk_is_builtin(const lsthunk_t* thunk) {
  return thunk && thunk->lt_type == LSTTYPE_BUILTIN;
}

lstbuiltin_func_t lsthunk_get_builtin_func(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BUILTIN) ? thunk->lt_builtin->lti_func : NULL;
}

void* lsthunk_get_builtin_data(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BUILTIN) ? thunk->lt_builtin->lti_data : NULL;
}

// cppcheck-suppress unusedFunction
const lsstr_t* lsthunk_get_builtin_name(const lsthunk_t* thunk) {
  return (thunk && thunk->lt_type == LSTTYPE_BUILTIN) ? thunk->lt_builtin->lti_name : NULL;
}

lsthunk_t* lsprog_eval(const lsprog_t* prog, lstenv_t* tenv) {
  lsthunk_t* thunk = lsthunk_new_expr(lsprog_get_expr(prog), tenv);
  if (thunk == NULL)
    return NULL;
  return lsthunk_eval0(thunk);
}

typedef enum lsprint_mode { LSPM_SHARROW, LSPM_ASIS, LSPM_DEEP } lsprint_mode_t;

typedef struct lsthunk_colle {
  // thunk entry
  lsthunk_t* ltc_thunk;
  // thunk id
  lssize_t ltc_id;
  // duplicated count
  lssize_t ltc_count;
  // minimal level
  lssize_t ltc_level;
  // next thunk entry
  struct lsthunk_colle* ltc_next;
} lsthunk_colle_t;

lsthunk_colle_t** lsthunk_colle_find(lsthunk_colle_t** pcolle, const lsthunk_t* thunk) {
  assert(pcolle != NULL);
  while (*pcolle != NULL) {
    if ((*pcolle)->ltc_thunk == thunk)
      break;
    pcolle = &(*pcolle)->ltc_next;
  }
  return pcolle;
}

static lsthunk_colle_t* lsthunk_colle_new(lsthunk_t* thunk, lsthunk_colle_t* head, lssize_t* pid,
                                          lssize_t level, lsprint_mode_t mode) {
  switch (mode) {
  case LSPM_SHARROW:
    break;
  case LSPM_ASIS:
    thunk = thunk->lt_whnf != NULL ? thunk->lt_whnf : thunk;
    break;
  case LSPM_DEEP:
    thunk = lsthunk_eval0(thunk);
    break;
  }
  lsthunk_colle_t** pcolle = lsthunk_colle_find(&head, thunk);
  if (*pcolle != NULL) {
    (*pcolle)->ltc_id = (*pid)++;
    (*pcolle)->ltc_count++;
    if ((*pcolle)->ltc_level > level)
      (*pcolle)->ltc_level = level;
    return head;
  }
  *pcolle              = lsmalloc(sizeof(lsthunk_colle_t));
  (*pcolle)->ltc_thunk = thunk;
  (*pcolle)->ltc_id    = 0;
  (*pcolle)->ltc_count = 1;
  (*pcolle)->ltc_level = level;
  (*pcolle)->ltc_next  = NULL;
  switch (thunk->lt_type) {
  case LSTTYPE_ALGE:
    for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++)
      head = lsthunk_colle_new(thunk->lt_alge.lta_args[i], head, pid, level + 1, mode);
    break;
  case LSTTYPE_APPL:
    head = lsthunk_colle_new(thunk->lt_appl.lta_func, head, pid, level + 1, mode);
    for (lssize_t i = 0; i < thunk->lt_appl.lta_argc; i++)
      head = lsthunk_colle_new(thunk->lt_appl.lta_args[i], head, pid, level + 1, mode);
    break;
  case LSTTYPE_CHOICE:
    head = lsthunk_colle_new(thunk->lt_choice.ltc_left, head, pid, level + 1, mode);
    head = lsthunk_colle_new(thunk->lt_choice.ltc_right, head, pid, level + 1, mode);
    break;
  case LSTTYPE_LAMBDA:
    head = lsthunk_colle_new(thunk->lt_lambda.ltl_body, head, pid, level + 1, mode);
    break;
  case LSTTYPE_BOTTOM: {
    lssize_t          ac = lsthunk_bottom_get_argc(thunk);
    lsthunk_t* const* xs = lsthunk_bottom_get_args(thunk);
    for (lssize_t i = 0; i < ac; i++)
      head = lsthunk_colle_new(xs[i], head, pid, level + 1, mode);
    break;
  }
  case LSTTYPE_REF:
  case LSTTYPE_INT:
  case LSTTYPE_STR:
  case LSTTYPE_SYMBOL:
  case LSTTYPE_BUILTIN:
    break;
  }
  return head;
}

static void lsthunk_print_internal(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk,
                                   lssize_t level, lsthunk_colle_t* colle, lsprint_mode_t mode,
                                   int force_print) {
  switch (mode) {
  case LSPM_SHARROW:
    break;
  case LSPM_ASIS:
    thunk = thunk->lt_whnf != NULL ? thunk->lt_whnf : thunk;
    break;
  case LSPM_DEEP:
    thunk = lsthunk_eval0(thunk);
    break;
  }
  int              has_dup     = 0;
  lsthunk_colle_t* colle_found = NULL;
  for (lsthunk_colle_t* c = colle; c != NULL; c = c->ltc_next) {
    if (c->ltc_count > 1 && c->ltc_level == level) {
      has_dup = 1;
      if (colle_found != NULL)
        break;
    }
    if (c->ltc_thunk == thunk) {
      colle_found = c;
      if (has_dup)
        break;
    }
  }
  assert(colle_found != NULL);

  if (has_dup)
    lsprintf(fp, ++indent, "(\n");

  if (!force_print && colle_found->ltc_count > 1)
    lsprintf(fp, 0, "~__ref%u", colle_found->ltc_id);
  else
    switch (thunk->lt_type) {
    case LSTTYPE_ALGE:
      // Pretty-print proper lists (x : y : [] => [x, y])
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
          thunk->lt_alge.lta_argc == 2) {
        // Collect elements along cons-chain with a safety cap
        lssize_t         cap = 8, n = 0;
        int              aborted   = 0;
        lsthunk_t**      elems     = lsmalloc(sizeof(lsthunk_t*) * cap);
        const lsthunk_t* cur       = thunk;
        lssize_t         max_steps = 4096;
        {
          const char* env_ms = getenv("LAZYSCRIPT_LIST_PRINT_MAX_STEPS");
          if (env_ms && *env_ms) {
            long v = strtol(env_ms, NULL, 10);
            if (v > 0 && v < 1000000)
              max_steps = (lssize_t)v;
          }
        }
        lssize_t         steps     = 0;
        while (1) {
          if (steps++ > max_steps) {
            aborted = 1;
            break;
          }
          const lsthunk_t* curv = lsthunk_eval0((lsthunk_t*)cur);
          if (!(curv->lt_type == LSTTYPE_ALGE &&
                lsstrcmp(curv->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
                curv->lt_alge.lta_argc == 2))
            break;
          if (n >= cap) {
            cap *= 2;
            lsthunk_t** tmp = lsmalloc(sizeof(lsthunk_t*) * cap);
            for (lssize_t i = 0; i < n; i++)
              tmp[i] = elems[i];
            elems = tmp;
          }
          elems[n++] = curv->lt_alge.lta_args[0];
          cur        = curv->lt_alge.lta_args[1];
        }
        if (!aborted) {
          const lsthunk_t* tailv  = lsthunk_eval0((lsthunk_t*)cur);
          int              is_nil = (tailv->lt_type == LSTTYPE_ALGE &&
                        lsstrcmp(tailv->lt_alge.lta_constr, lsstr_cstr("[]")) == 0 &&
                        tailv->lt_alge.lta_argc == 0);
          if (is_nil) {
            // Render as [e0, e1, ...]
            lsprintf(fp, 0, "[");
            for (lssize_t i = 0; i < n; i++) {
              if (i > 0)
                lsprintf(fp, 0, ", ");
              lsthunk_t* e = lsthunk_eval0(elems[i]);
              if (e->lt_type == LSTTYPE_INT) {
                lsint_print(fp, LSPREC_LOWEST, indent, e->lt_int);
              } else if (e->lt_type == LSTTYPE_STR) {
                lsstr_print_bare(fp, LSPREC_LOWEST, indent, e->lt_str);
              } else if (e->lt_type == LSTTYPE_ALGE &&
                         lsstrcmp(e->lt_alge.lta_constr, lsstr_cstr("()")) == 0 &&
                         e->lt_alge.lta_argc == 0) {
                lsprintf(fp, 0, "()");
              } else {
                lsthunk_print_internal(fp, LSPREC_LOWEST, indent, e, level + 1, colle, mode, 0);
              }
            }
            lsprintf(fp, 0, "]");
            return;
          }
        }
        // else fallthrough to default ':' printing below
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr("[]")) == 0) {
        lsprintf(fp, 0, "[");
        for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
          if (i > 0)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent, thunk->lt_alge.lta_args[i], level + 1,
                                 colle, mode, 0);
        }
        lsprintf(fp, 0, "]");
        return;
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(",")) == 0) {
        lsprintf(fp, 0, "(");
        for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
          if (i > 0)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent, thunk->lt_alge.lta_args[i], level + 1,
                                 colle, mode, 0);
        }
        lsprintf(fp, 0, ")");
        return;
      }
      if (lsstrcmp(thunk->lt_alge.lta_constr, lsstr_cstr(":")) == 0 &&
          thunk->lt_alge.lta_argc == 2) {
        if (prec > LSPREC_CONS)
          lsprintf(fp, 0, "(");
        lsthunk_print_internal(fp, LSPREC_CONS + 1, indent, thunk->lt_alge.lta_args[0], level + 1,
                               colle, mode, 0);
        lsprintf(fp, 0, " : ");
        lsthunk_print_internal(fp, LSPREC_CONS, indent, thunk->lt_alge.lta_args[1], level + 1,
                               colle, mode, 0);
        if (prec > LSPREC_CONS)
          lsprintf(fp, 0, ")");
        return;
      }
      if (thunk->lt_alge.lta_argc == 0) {
        lsstr_print_bare(fp, prec, indent, thunk->lt_alge.lta_constr);
        return;
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, indent, "(");
      lsstr_print_bare(fp, prec, indent, thunk->lt_alge.lta_constr);
      for (lssize_t i = 0; i < thunk->lt_alge.lta_argc; i++) {
        lsprintf(fp, indent, " ");
        lsthunk_print_internal(fp, LSPREC_APPL + 1, indent, thunk->lt_alge.lta_args[i], level + 1,
                               colle, mode, 0);
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, 0, ")");
      break;
    case LSTTYPE_APPL:
      if (thunk->lt_appl.lta_argc == 0) {
        lsthunk_print_internal(fp, prec, indent, thunk->lt_appl.lta_func, level + 1, colle, mode,
                               0);
        return;
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, 0, "(");
      lsthunk_print(fp, LSPREC_APPL + 1, indent, thunk->lt_appl.lta_func);
      for (lssize_t i = 0; i < thunk->lt_appl.lta_argc; i++) {
        lsprintf(fp, indent, " ");
        lsthunk_print_internal(fp, LSPREC_APPL + 1, indent, thunk->lt_appl.lta_args[i], level + 1,
                               colle, mode, 0);
      }
      if (prec > LSPREC_APPL)
        lsprintf(fp, indent, ")");
      break;
    case LSTTYPE_CHOICE:
      if (prec > LSPREC_CHOICE)
        lsprintf(fp, 0, "(");
      lsthunk_print_internal(fp, LSPREC_CHOICE + 1, indent, thunk->lt_choice.ltc_left, level + 1,
                             colle, mode, 0);
      // Operator print: show symbol based on kind (debug printer)
      const char* op_sym_dbg = " || ";
      if (thunk->lt_choice.ltc_kind == 1)
        op_sym_dbg = " | ";
      else if (thunk->lt_choice.ltc_kind == 3)
        op_sym_dbg = " ^| ";
      lsprintf(fp, 0, "%s", op_sym_dbg);
      lsthunk_print_internal(fp, LSPREC_CHOICE, indent, thunk->lt_choice.ltc_right, level + 1,
                             colle, mode, 0);
      if (prec > LSPREC_CHOICE)
        lsprintf(fp, 0, ")");
      break;
    case LSTTYPE_LAMBDA:
      if (prec > LSPREC_LAMBDA)
        lsprintf(fp, indent, "(");
      lsprintf(fp, indent, "\\");
      lstpat_print(fp, LSPREC_APPL + 1, indent, thunk->lt_lambda.ltl_param);
      lsprintf(fp, indent, " -> ");
      lsthunk_print_internal(fp, LSPREC_LAMBDA, indent, thunk->lt_lambda.ltl_body, level + 1, colle,
                             mode, 0);
      if (prec > LSPREC_LAMBDA)
        lsprintf(fp, indent, ")");
      break;
    case LSTTYPE_REF:
      lsref_print(fp, prec, indent, thunk->lt_ref.ltr_ref);
      break;
    case LSTTYPE_INT:
      lsint_print(fp, prec, indent, thunk->lt_int);
      break;
    case LSTTYPE_STR:
      lsstr_print(fp, prec, indent, thunk->lt_str);
      break;
    case LSTTYPE_SYMBOL:
      // Print symbol as bare (already includes leading dot)
      lsstr_print_bare(fp, prec, indent, thunk->lt_symbol);
      break;
    case LSTTYPE_BOTTOM: {
      // ASCII representation
      lsprintf(fp, indent, "<bottom msg=\"");
      const char* msg = lsthunk_bottom_get_message(thunk);
      if (msg)
        fprintf(fp, "%s", msg);
      lsprintf(fp, 0, "\" at ");
      lsloc_print(fp, lsthunk_bottom_get_loc(thunk));
      lssize_t ac = lsthunk_bottom_get_argc(thunk);
      if (ac > 0) {
        lsprintf(fp, 0, " args=[");
        for (lssize_t i = 0; i < ac; i++) {
          if (i)
            lsprintf(fp, 0, ", ");
          lsthunk_print_internal(fp, LSPREC_LOWEST, indent, thunk->lt_bottom.lt_rel.lbr_args[i],
                                 level + 1, colle, mode, 0);
        }
        lsprintf(fp, 0, "]");
      }
      lsprintf(fp, 0, ">");
      break;
    }
    case LSTTYPE_BUILTIN:
      lsprintf(fp, indent, "~%s{-builtin/%d-}", lsstr_get_buf(thunk->lt_builtin->lti_name),
               thunk->lt_builtin->lti_arity);
      break;
    }
  if (has_dup) {
    for (lsthunk_colle_t* c = colle; c != NULL; c = c->ltc_next) {
      if (c->ltc_level == level && c->ltc_count > 1) {
        lsprintf(fp, indent, ";\n~__ref%u = ", c->ltc_id);
        lsthunk_print_internal(fp, LSPREC_LOWEST, indent, c->ltc_thunk, level + 1, colle, mode, 1);
      }
    }
    lsprintf(fp, --indent, "\n)");
  }
}

void lsthunk_print(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk) {
  lssize_t         id    = 0;
  lsthunk_colle_t* colle = lsthunk_colle_new(thunk, NULL, &id, 0, 0);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_SHARROW, 0);
}

void lsthunk_dprint(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk) {
  lssize_t         id    = 0;
  lsthunk_colle_t* colle = lsthunk_colle_new(thunk, NULL, &id, 0, 1);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_ASIS, 0);
}

void lsthunk_deep_print(FILE* fp, lsprec_t prec, int indent, lsthunk_t* thunk) {
  lssize_t         id    = 0;
  lsthunk_colle_t* colle = lsthunk_colle_new(thunk, NULL, &id, 0, 2);
  lsthunk_print_internal(fp, prec, indent, thunk, 0, colle, LSPM_DEEP, 0);
}

lsthunk_t* lsthunk_clone(lsthunk_t* thunk) {
  // Thunks are treated as immutable graph nodes managed by GC.
  // Returning the same pointer is sufficient for now.
  return thunk;
}

// --- Capture substitution -------------------------------------------------

typedef struct subst_entry {
  const lsthunk_t*    key;
  lsthunk_t*          val;
  struct subst_entry* next;
} subst_entry_t;

// Map for inlining bind references (to preserve recursion when cloning RHS)
typedef struct inline_refmap {
  const lstref_target_t* target; // identity of the bind being inlined
  lsthunk_t*             clone;  // cloned thunk placeholder/result
  struct inline_refmap*  next;
} inline_refmap_t;

// Lightweight checker: does thunk tree contain a reference to the current lambda parameter?
// It walks without allocation and stops on first positive hit. We only need to detect
// existence to decide whether to inline a local BIND for capture.
static int param_contains_name(lstpat_t* param, const lsstr_t* name) {
  if (!param || !name)
    return 0;
  switch (lstpat_get_type(param)) {
  case LSPTYPE_REF: {
    const lsstr_t* nm = lstpat_get_refname(param);
    return nm && (lsstrcmp(nm, name) == 0);
  }
  case LSPTYPE_ALGE: {
    lssize_t n = lstpat_get_argc(param);
    lstpat_t* const* xs = lstpat_get_args(param);
    for (lssize_t i = 0; i < n; i++)
      if (param_contains_name(xs[i], name))
        return 1;
    return 0;
  }
  case LSPTYPE_AS:
    return param_contains_name(lstpat_get_ref(param), name) ||
           param_contains_name(lstpat_get_aspattern(param), name);
  case LSPTYPE_OR:
    // Conservative: either branch may bind the name
    return param_contains_name(lstpat_get_or_left(param), name) ||
           param_contains_name(lstpat_get_or_right(param), name);
  case LSPTYPE_CARET:
    return param_contains_name((lstpat_t*)lspat_get_caret_inner((const lspat_t*)param), name);
  default:
    return 0;
  }
}

static lsthunk_t* param_get_bound_by_name(lstpat_t* param, const lsstr_t* name) {
  if (!param || !name)
    return NULL;
  switch (lstpat_get_type(param)) {
  case LSPTYPE_REF: {
    const lsstr_t* nm = lstpat_get_refname(param);
    if (nm && (lsstrcmp(nm, name) == 0))
      return lstpat_get_refbound(param);
    return NULL;
  }
  case LSPTYPE_ALGE: {
    lssize_t n = lstpat_get_argc(param);
    lstpat_t* const* xs = lstpat_get_args(param);
    for (lssize_t i = 0; i < n; i++) {
      lsthunk_t* v = param_get_bound_by_name(xs[i], name);
      if (v)
        return v;
    }
    return NULL;
  }
  case LSPTYPE_AS: {
    lsthunk_t* v = param_get_bound_by_name(lstpat_get_ref(param), name);
    if (v)
      return v;
    return param_get_bound_by_name(lstpat_get_aspattern(param), name);
  }
  case LSPTYPE_OR: {
    // If OR was used, only one branch binds; prefer the one with a bound present
    lsthunk_t* v = param_get_bound_by_name(lstpat_get_or_left(param), name);
    if (v)
      return v;
    return param_get_bound_by_name(lstpat_get_or_right(param), name);
  }
  case LSPTYPE_CARET:
    return param_get_bound_by_name((lstpat_t*)lspat_get_caret_inner((const lspat_t*)param), name);
  default:
    return NULL;
  }
}

static int contains_param_ref(lsthunk_t* t, lstpat_t* param) {
  if (!t)
    return 0;
  switch (t->lt_type) {
  case LSTTYPE_REF: {
    lstref_target_t* target = t->lt_ref.ltr_target;
    if (!target && t->lt_ref.ltr_env)
      target = lstenv_get(t->lt_ref.ltr_env, lsref_get_name(t->lt_ref.ltr_ref));
    if (target) {
      lstref_target_origin_t* org = target->lrt_origin;
      if (org && org->lrto_type == LSTRTYPE_LAMBDA && org->lrto_lambda.ltl_param == param)
        return 1;
      return 0;
    }
    // Fallback: if target is unresolved, check by name occurrence in the param pattern.
    const lsstr_t* nm = t->lt_ref.ltr_ref ? lsref_get_name(t->lt_ref.ltr_ref) : NULL;
    return param_contains_name(param, nm);
  }
  case LSTTYPE_ALGE: {
    for (lssize_t i = 0; i < t->lt_alge.lta_argc; i++)
      if (contains_param_ref(t->lt_alge.lta_args[i], param))
        return 1;
    return 0;
  }
  case LSTTYPE_APPL: {
    if (contains_param_ref(t->lt_appl.lta_func, param))
      return 1;
    for (lssize_t i = 0; i < t->lt_appl.lta_argc; i++)
      if (contains_param_ref(t->lt_appl.lta_args[i], param))
        return 1;
    return 0;
  }
  case LSTTYPE_CHOICE:
    return contains_param_ref(t->lt_choice.ltc_left, param) ||
           contains_param_ref(t->lt_choice.ltc_right, param);
  case LSTTYPE_LAMBDA:
    // Refs to the outer parameter can appear inside nested lambdas' bodies
    return contains_param_ref(t->lt_lambda.ltl_body, param);
  default:
    return 0;
  }
}

static lsthunk_t* subst_lookup(subst_entry_t* m, const lsthunk_t* k) {
  for (; m; m = m->next)
    if (m->key == k)
      return m->val;
  return NULL;
}

static subst_entry_t* subst_bind(subst_entry_t* m, const lsthunk_t* k, lsthunk_t* v) {
  subst_entry_t* e = lsmalloc(sizeof(subst_entry_t));
  e->key           = k;
  e->val           = v;
  e->next          = m;
  return e;
}

static inline_refmap_t* inline_refmap_bind(inline_refmap_t* m, const lstref_target_t* target,
                                           lsthunk_t* clone) {
  inline_refmap_t* e = lsmalloc(sizeof(inline_refmap_t));
  e->target          = target;
  e->clone           = clone;
  e->next            = m;
  return e;
}

static lsthunk_t* lsthunk_subst_param_rec(lsthunk_t* t, lstpat_t* param, subst_entry_t** pmemo,
                                          inline_refmap_t** prefmap) {
  if (t == NULL)
    return NULL;
  lsthunk_t* memo = subst_lookup(*pmemo, t);
  if (memo)
    return memo;

  switch (t->lt_type) {
  case LSTTYPE_REF: {
    lstref_target_t* target = t->lt_ref.ltr_target;
    // If the reference target hasn't been resolved yet, attempt a lazy
    // resolution against the captured environment so we can identify whether
    // this ref points to the current lambda parameter and capture it.
    if (!target && t->lt_ref.ltr_env) {
      // Resolve on-the-fly from env for capture purposes, but do not cache into the node
      target = lstenv_get(t->lt_ref.ltr_env, lsref_get_name(t->lt_ref.ltr_ref));
    }
  if (target) {
      // If this reference belongs to the same lambda parameter (including any subpattern
      // inside the parameter), capture the currently bound thunk for that specific ref.
      lstref_target_origin_t* org = target->lrt_origin;
      if (org && org->lrto_type == LSTRTYPE_LAMBDA && org->lrto_lambda.ltl_param == param) {
        lstpat_t*  pref  = target->lrt_pat; // the concrete ref node within the param pattern
        lsthunk_t* bound = pref ? lstpat_get_refbound(pref) : NULL;
        if (bound) {
          const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
          if (dbg && *dbg) {
            lsprintf(stderr, 0, "[capture] ref to param captured: pref=%p -> %p\n",
                     (void*)pref, (void*)bound);
          }
          return bound;
        }
      }
      // Inline local bind references (e.g., ~go) only when RHS actually depends on the
      // current parameter binding; otherwise leave as ref to avoid unnecessary duplication/loops.
      if (org && org->lrto_type == LSTRTYPE_BIND) {
        // Optional gate: disable inlining of local BINDs entirely to validate
        // recursion hypotheses (set LAZYSCRIPT_CAPTURE_INLINE_BINDS=0)
        {
          const char* gate = getenv("LAZYSCRIPT_CAPTURE_INLINE_BINDS");
          if (gate && (gate[0] == '0')) {
            const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
            if (dbg && *dbg) {
              lsprintf(stderr, 0, "[capture] inline(bind) disabled by env; keep ref %p (name=",
                       (void*)t);
              if (t->lt_ref.ltr_ref)
                lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(t->lt_ref.ltr_ref));
              lsprintf(stderr, 0, ")\n");
            }
            return t; // keep as reference; no inlining
          }
        }
        // If we are already inlining this bind, return the clone to keep recursion well-founded
        for (inline_refmap_t* e = *prefmap; e != NULL; e = e->next) {
          if (e->target == target && e->clone != NULL)
            return e->clone;
        }
        lsthunk_t* rhs = org->lrto_bind.ltb_rhs;
        // Quick dependency test: skip inlining if RHS has no reference to the current param
        if (!contains_param_ref(rhs, param)) {
          const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
          if (dbg && *dbg) {
            lsprintf(stderr, 0, "[capture] keep bind ref (no param dep) %p name=", (void*)target);
            if (t->lt_ref.ltr_ref)
              lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(t->lt_ref.ltr_ref));
            lsprintf(stderr, 0, "\n");
          }
          return t;
        }
        if (rhs && rhs->lt_type == LSTTYPE_LAMBDA) {
          // Create a lambda shell and register it before descending to support self-recursion
          lsthunk_t* nt           = lsmalloc(sizeof(lsthunk_t));
          nt->lt_type             = LSTTYPE_LAMBDA;
          nt->lt_whnf             = nt;
          nt->lt_trace_id         = -1;
          nt->lt_lambda.ltl_param = rhs->lt_lambda.ltl_param;
          *prefmap                = inline_refmap_bind(*prefmap, target, nt);
          nt->lt_lambda.ltl_body  = lsthunk_subst_param_rec(rhs->lt_lambda.ltl_body, param, pmemo,
                                                           prefmap);
          const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
          if (dbg && *dbg) {
            lsprintf(stderr, 0, "[capture] inline bind lambda %p -> %p\n", (void*)target,
                     (void*)nt);
          }
          return nt;
        } else if (rhs && rhs->lt_type == LSTTYPE_CHOICE) {
          // Create a choice shell first to break recursive references to this bind within its RHS
          lsthunk_t* nt           = lsmalloc(sizeof(lsthunk_t));
          nt->lt_type             = LSTTYPE_CHOICE;
          nt->lt_whnf             = NULL;
          nt->lt_trace_id         = -1;
          *prefmap                = inline_refmap_bind(*prefmap, target, nt);
          // Fill children under the mapping so internal refs to this bind resolve to nt
          nt->lt_choice.ltc_left  = lsthunk_subst_param_rec(rhs->lt_choice.ltc_left, param, pmemo,
                                                           prefmap);
          nt->lt_choice.ltc_right = lsthunk_subst_param_rec(rhs->lt_choice.ltc_right, param, pmemo,
                                                            prefmap);
          nt->lt_choice.ltc_kind  = rhs->lt_choice.ltc_kind; // preserve operator semantics
          const char* dbg         = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
          if (dbg && *dbg) {
            lsprintf(stderr, 0, "[capture] inline bind choice %p -> %p\n", (void*)target,
                     (void*)nt);
          }
          return nt;
        } else if (rhs) {
          // Non-lambda RHS: create a shell clone first, register it to break cycles,
          // then fill it recursively.
          lsthunk_t* nt = NULL;
          switch (rhs->lt_type) {
          case LSTTYPE_ALGE: {
            lssize_t n = rhs->lt_alge.lta_argc;
            nt         = lsmalloc(lssizeof(lsthunk_t, lt_alge) + n * sizeof(lsthunk_t*));
            nt->lt_type            = LSTTYPE_ALGE;
            nt->lt_whnf            = nt;
            nt->lt_trace_id        = -1;
            nt->lt_alge.lta_constr = rhs->lt_alge.lta_constr;
            nt->lt_alge.lta_argc   = n;
            // Register before filling to handle self-references
            *prefmap = inline_refmap_bind(*prefmap, target, nt);
            for (lssize_t i = 0; i < n; i++)
              nt->lt_alge.lta_args[i] =
                  lsthunk_subst_param_rec(rhs->lt_alge.lta_args[i], param, pmemo, prefmap);
            break;
          }
          case LSTTYPE_APPL: {
            lssize_t n = rhs->lt_appl.lta_argc;
            nt         = lsmalloc(lssizeof(lsthunk_t, lt_appl) + n * sizeof(lsthunk_t*));
            nt->lt_type          = LSTTYPE_APPL;
            nt->lt_whnf          = NULL;
            nt->lt_trace_id      = -1;
            // Register before filling
            *prefmap             = inline_refmap_bind(*prefmap, target, nt);
            nt->lt_appl.lta_func = lsthunk_subst_param_rec(rhs->lt_appl.lta_func, param, pmemo,
                                                           prefmap);
            nt->lt_appl.lta_argc = n;
            for (lssize_t i = 0; i < n; i++)
              nt->lt_appl.lta_args[i] =
                  lsthunk_subst_param_rec(rhs->lt_appl.lta_args[i], param, pmemo, prefmap);
            break;
          }
          case LSTTYPE_CHOICE: {
            // Handled above, but keep as safety
            nt                     = lsmalloc(sizeof(lsthunk_t));
            nt->lt_type            = LSTTYPE_CHOICE;
            nt->lt_whnf            = NULL;
            nt->lt_trace_id        = -1;
            *prefmap               = inline_refmap_bind(*prefmap, target, nt);
            nt->lt_choice.ltc_left =
                lsthunk_subst_param_rec(rhs->lt_choice.ltc_left, param, pmemo, prefmap);
            nt->lt_choice.ltc_right =
                lsthunk_subst_param_rec(rhs->lt_choice.ltc_right, param, pmemo, prefmap);
            nt->lt_choice.ltc_kind = rhs->lt_choice.ltc_kind;
            break;
          }
          case LSTTYPE_LAMBDA: {
            // Handled in lambda branch; fallthrough should not happen
            nt                     = lsmalloc(sizeof(lsthunk_t));
            nt->lt_type            = LSTTYPE_LAMBDA;
            nt->lt_whnf            = nt;
            nt->lt_trace_id        = -1;
            nt->lt_lambda.ltl_param = rhs->lt_lambda.ltl_param;
            *prefmap               = inline_refmap_bind(*prefmap, target, nt);
            nt->lt_lambda.ltl_body =
                lsthunk_subst_param_rec(rhs->lt_lambda.ltl_body, param, pmemo, prefmap);
            break;
          }
          case LSTTYPE_REF: {
            // Register a temporary self-ref to break cycles, then substitute the ref
            // (which may resolve to param-bound thunk or other inlined binds)
            nt       = rhs; // use as-is; substitution below will process it
            *prefmap = inline_refmap_bind(*prefmap, target, nt);
            nt       = lsthunk_subst_param_rec(rhs, param, pmemo, prefmap);
            // Update mapping to the resolved clone
            *prefmap = inline_refmap_bind(*prefmap, target, nt);
            break;
          }
          case LSTTYPE_BOTTOM: {
            lssize_t rc = rhs->lt_bottom.lt_rel.lbr_argc;
            nt         = lsthunk_alloc_bottom(rhs->lt_bottom.lt_msg,
                                              lsthunk_bottom_get_loc(rhs), rc);
            *prefmap   = inline_refmap_bind(*prefmap, target, nt);
            for (lssize_t i = 0; i < rc; i++)
              lsthunk_set_bottom_related(
                  nt, i,
                  lsthunk_subst_param_rec(rhs->lt_bottom.lt_rel.lbr_args[i], param, pmemo,
                                          prefmap));
            break;
          }
          case LSTTYPE_INT:
          case LSTTYPE_STR:
          case LSTTYPE_SYMBOL:
          case LSTTYPE_BUILTIN:
          default:
            // Leaf or unsupported: reuse as-is
            nt       = rhs;
            *prefmap = inline_refmap_bind(*prefmap, target, nt);
            break;
          }
          const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
          if (dbg && *dbg) {
            lsprintf(stderr, 0, "[capture] inline bind non-lambda %p -> %p\n", (void*)target,
                     (void*)nt);
          }
          return nt;
        }
      }
    }
    // Not resolved via target: attempt name-based capture if this ref is a param subpattern
    {
      const lsstr_t* nm = t->lt_ref.ltr_ref ? lsref_get_name(t->lt_ref.ltr_ref) : NULL;
      if (nm && param_contains_name(param, nm)) {
        lsthunk_t* bound = param_get_bound_by_name(param, nm);
        if (bound) {
          const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
          if (dbg && *dbg) {
            lsprintf(stderr, 0, "[capture] name-based capture %p name=", (void*)t);
            lsstr_print_bare(stderr, LSPREC_LOWEST, 0, nm);
            lsprintf(stderr, 0, " -> %p\n", (void*)bound);
          }
          return bound;
        }
      }
    }
    // Not a param ref (or not bound yet): leave as-is
    {
      const char* dbg = getenv("LAZYSCRIPT_DEBUG_CAPTURE");
      if (dbg && *dbg) {
        lsprintf(stderr, 0, "[capture] skip ref %p (name=", (void*)t);
        if (t->lt_ref.ltr_ref)
          lsstr_print_bare(stderr, LSPREC_LOWEST, 0, lsref_get_name(t->lt_ref.ltr_ref));
        lsprintf(stderr, 0, ") target=%p)\n", (void*)target);
      }
    }
    return t;
  }
  case LSTTYPE_ALGE: {
    lssize_t   n           = t->lt_alge.lta_argc;
    lsthunk_t* nt          = lsmalloc(lssizeof(lsthunk_t, lt_alge) + n * sizeof(lsthunk_t*));
    nt->lt_type            = LSTTYPE_ALGE;
    nt->lt_whnf            = nt;
    nt->lt_trace_id        = -1;
    nt->lt_alge.lta_constr = t->lt_alge.lta_constr;
    nt->lt_alge.lta_argc   = n;
    *pmemo                 = subst_bind(*pmemo, t, nt);
    for (lssize_t i = 0; i < n; i++)
      nt->lt_alge.lta_args[i] =
          lsthunk_subst_param_rec(t->lt_alge.lta_args[i], param, pmemo, prefmap);
    return nt;
  }
  case LSTTYPE_APPL: {
    lssize_t   n         = t->lt_appl.lta_argc;
    lsthunk_t* nt        = lsmalloc(lssizeof(lsthunk_t, lt_appl) + n * sizeof(lsthunk_t*));
    nt->lt_type          = LSTTYPE_APPL;
    nt->lt_whnf          = NULL;
    nt->lt_trace_id      = -1;
    nt->lt_appl.lta_func = lsthunk_subst_param_rec(t->lt_appl.lta_func, param, pmemo, prefmap);
    nt->lt_appl.lta_argc = n;
    *pmemo               = subst_bind(*pmemo, t, nt);
    for (lssize_t i = 0; i < n; i++)
      nt->lt_appl.lta_args[i] =
          lsthunk_subst_param_rec(t->lt_appl.lta_args[i], param, pmemo, prefmap);
    return nt;
  }
  case LSTTYPE_CHOICE: {
    lsthunk_t* nt           = lsmalloc(sizeof(lsthunk_t));
    nt->lt_type             = LSTTYPE_CHOICE;
    nt->lt_whnf             = NULL;
    nt->lt_trace_id         = -1;
    *pmemo                  = subst_bind(*pmemo, t, nt);
    nt->lt_choice.ltc_left  = lsthunk_subst_param_rec(t->lt_choice.ltc_left, param, pmemo, prefmap);
    nt->lt_choice.ltc_right =
        lsthunk_subst_param_rec(t->lt_choice.ltc_right, param, pmemo, prefmap);
    // Preserve choice operator kind to keep evaluation semantics ('|' vs '||')
    nt->lt_choice.ltc_kind = t->lt_choice.ltc_kind;
    return nt;
  }
  case LSTTYPE_LAMBDA: {
    // Capture references to the outer parameter even across lambda boundaries by
    // substituting inside the lambda body. Keep the parameter pattern as-is.
    lsthunk_t* nt           = lsmalloc(sizeof(lsthunk_t));
    nt->lt_type             = LSTTYPE_LAMBDA;
    nt->lt_whnf             = nt;
    nt->lt_trace_id         = -1;
    nt->lt_lambda.ltl_param = t->lt_lambda.ltl_param;
  *pmemo                  = subst_bind(*pmemo, t, nt);
  nt->lt_lambda.ltl_body  =
    lsthunk_subst_param_rec(t->lt_lambda.ltl_body, param, pmemo, prefmap);
    return nt;
  }
  case LSTTYPE_INT:
  case LSTTYPE_STR:
  case LSTTYPE_BUILTIN:
  default:
    return t;
  }
}

lsthunk_t* lsthunk_subst_param(lsthunk_t* thunk, lstpat_t* param) {
  subst_entry_t* memo = NULL;
  inline_refmap_t* rmap = NULL;
  return lsthunk_subst_param_rec(thunk, param, &memo, &rmap);
}

// --- Thunk Binary (LSTB) I/O (subset v0.1) -------------------------------

// Note: For v0.1 implementation we support a safe subset of kinds that do not
// depend on external environments or ref-targets: INT, STR, SYMBOL, ALGE, BOTTOM.
// Other kinds (APPL/LAMBDA/CHOICE/REF/BUILTIN) currently return an error.

// Small helpers: ULEB128/SLEB128
static int write_varuint(FILE* fp, uint64_t v) {
  while (1) {
    uint8_t b = (uint8_t)(v & 0x7fu);
    v >>= 7;
    if (v) {
      b |= 0x80u;
    }
    if (fputc((int)b, fp) == EOF)
      return -1;
    if (!v)
      break;
  }
  return 0;
}

static int write_varint(FILE* fp, int64_t val) {
  // SLEB128 encoding
  int more = 1;
  while (more) {
    uint8_t byte = (uint8_t)(val & 0x7f);
    int     sign = (val & ~0x3f) == 0 || (val & ~0x3f) == ~0ULL; // determine termination
    val >>= 7;
    if ((val == 0 && (byte & 0x40) == 0) || (val == -1 && (byte & 0x40) != 0)) {
      more = 0;
    } else {
      byte |= 0x80;
    }
    if (fputc((int)byte, fp) == EOF)
      return -1;
  }
  return 0;
}

static int read_varuint(FILE* fp, uint64_t* out) {
  uint64_t result = 0;
  int      shift  = 0;
  int      ch;
  do {
    ch = fgetc(fp);
    if (ch == EOF)
      return -1;
    uint8_t byte = (uint8_t)ch;
    result |= ((uint64_t)(byte & 0x7f)) << shift;
    shift += 7;
    if (!(byte & 0x80))
      break;
    if (shift > 63)
      return -1;
  } while (1);
  *out = result;
  return 0;
}

static int read_varint(FILE* fp, int64_t* out) {
  uint64_t result = 0;
  int      shift  = 0;
  int      ch;
  uint8_t  byte;
  do {
    ch = fgetc(fp);
    if (ch == EOF)
      return -1;
    byte = (uint8_t)ch;
    result |= ((uint64_t)(byte & 0x7f)) << shift;
    shift += 7;
    if (!(byte & 0x80))
      break;
    if (shift > 63)
      return -1;
  } while (1);
  // sign extend
  if (shift < 64 && (byte & 0x40))
    result |= (~UINT64_C(0)) << shift;
  *out = (int64_t)result;
  return 0;
}

// Simple pointer->index map for traversal (linear scan for now)
typedef struct {
  lsthunk_t** items;
  lssize_t    size;
  lssize_t    cap;
} thvec_t;

static void thvec_init(thvec_t* v) {
  v->items = NULL;
  v->size  = 0;
  v->cap   = 0;
}
static void thvec_free(thvec_t* v) {
  if (v->items)
    free(v->items);
  v->items = NULL;
  v->size = v->cap = 0;
}
static int thvec_push(thvec_t* v, lsthunk_t* t) {
  if (v->size == v->cap) {
    lssize_t    ncap = v->cap ? v->cap * 2 : 16;
    lsthunk_t** ni   = (lsthunk_t**)realloc(v->items, sizeof(lsthunk_t*) * (size_t)ncap);
    if (!ni)
      return -1;
    v->items = ni;
    v->cap   = ncap;
  }
  v->items[v->size++] = t;
  return 0;
}
static lssize_t thvec_index_of(const thvec_t* v, const lsthunk_t* t) {
  for (lssize_t i = 0; i < v->size; i++)
    if (v->items[i] == t)
      return i;
  return -1;
}

static int collect_subset(thvec_t* order, lsthunk_t* t) {
  if (!t)
    return 0;
  if (thvec_index_of(order, t) >= 0)
    return 0;
  if (thvec_push(order, t) != 0)
    return -1;
  switch (t->lt_type) {
  case LSTTYPE_INT:
  case LSTTYPE_STR:
  case LSTTYPE_SYMBOL:
  case LSTTYPE_BOTTOM:
    return 0;
  case LSTTYPE_ALGE:
    for (lssize_t i = 0; i < t->lt_alge.lta_argc; i++) {
      if (collect_subset(order, t->lt_alge.lta_args[i]) != 0)
        return -2;
    }
    return 0;
  default:
    return -100 - (int)t->lt_type;
  }
}

// Simple string pool for STRING_POOL and SYMBOL_POOL
typedef struct {
  const char** items;
  lssize_t     size;
  lssize_t     cap;
} strpool_t;
static void strpool_init(strpool_t* p) {
  p->items = NULL;
  p->size  = 0;
  p->cap   = 0;
}
static void strpool_free(strpool_t* p) {
  if (p->items)
    free(p->items);
  p->items = NULL;
  p->size = p->cap = 0;
}
static lssize_t strpool_index_of(const strpool_t* p, const char* s) {
  for (lssize_t i = 0; i < p->size; i++)
    if (strcmp(p->items[i], s) == 0)
      return i;
  return -1;
}
static int strpool_add(strpool_t* p, const char* s, lssize_t* out_id) {
  lssize_t idx = strpool_index_of(p, s);
  if (idx >= 0) {
    if (out_id)
      *out_id = idx;
    return 0;
  }
  if (p->size == p->cap) {
    lssize_t     ncap = p->cap ? p->cap * 2 : 32;
    const char** ni   = (const char**)realloc(p->items, sizeof(const char*) * (size_t)ncap);
    if (!ni)
      return -1;
    p->items = ni;
    p->cap   = ncap;
  }
  p->items[p->size] = s;
  if (out_id)
    *out_id = p->size;
  p->size++;
  return 0;
}

static int pool_collect_from_thunk(strpool_t* sp, strpool_t* yp, lsthunk_t* t) {
  if (!t)
    return 0;
  switch (t->lt_type) {
  case LSTTYPE_INT:
    return 0;
  case LSTTYPE_STR:
    return strpool_add(sp, lsstr_get_buf(t->lt_str), NULL);
  case LSTTYPE_SYMBOL:
    return strpool_add(yp, lsstr_get_buf(t->lt_symbol), NULL);
  case LSTTYPE_ALGE: {
    if (strpool_add(yp, lsstr_get_buf(t->lt_alge.lta_constr), NULL) != 0)
      return -1;
    for (lssize_t i = 0; i < t->lt_alge.lta_argc; i++)
      if (pool_collect_from_thunk(sp, yp, t->lt_alge.lta_args[i]) != 0)
        return -1;
    return 0;
  }
  case LSTTYPE_BOTTOM: {
    const char* m = t->lt_bottom.lt_msg ? t->lt_bottom.lt_msg : "";
    if (strpool_add(sp, m, NULL) != 0)
      return -1;
    for (lssize_t i = 0; i < t->lt_bottom.lt_rel.lbr_argc; i++)
      if (pool_collect_from_thunk(sp, yp, t->lt_bottom.lt_rel.lbr_args[i]) != 0)
        return -1;
    return 0;
  }
  default:
    return -1;
  }
}

static int write_pool(FILE* fp, const strpool_t* p) {
  if (write_varuint(fp, (uint64_t)p->size) != 0)
    return -1;
  for (lssize_t i = 0; i < p->size; i++) {
    const char* s = p->items[i];
    size_t      n = s ? strlen(s) : 0;
    if (write_varuint(fp, (uint64_t)n) != 0)
      return -1;
    if (n && fwrite(s, 1, n, fp) != n)
      return -1;
  }
  return 0;
}

static char** read_pool(FILE* fp, lssize_t* out_count) {
  uint64_t cnt64 = 0;
  if (read_varuint(fp, &cnt64) != 0)
    return NULL;
  lssize_t cnt = (lssize_t)cnt64;
  char**   arr = cnt ? (char**)malloc(sizeof(char*) * (size_t)cnt) : NULL;
  if (cnt && !arr)
    return NULL;
  for (lssize_t i = 0; i < cnt; i++) {
    uint64_t n64 = 0;
    if (read_varuint(fp, &n64) != 0) {
      for (lssize_t j = 0; j < i; j++)
        free(arr[j]);
      free(arr);
      return NULL;
    }
    size_t n = (size_t)n64;
    char*  s = n ? (char*)malloc(n + 1) : (char*)malloc(1);
    if (!s) {
      for (lssize_t j = 0; j < i; j++)
        free(arr[j]);
      free(arr);
      return NULL;
    }
    if (n) {
      if (fread(s, 1, n, fp) != n) {
        for (lssize_t j = 0; j < i; j++)
          free(arr[j]);
        free(s);
        free(arr);
        return NULL;
      }
    }
    s[n]   = '\0';
    arr[i] = s;
  }
  *out_count = cnt;
  return arr;
}

int lstb_write(FILE* fp, lsthunk_t* const* roots, lssize_t rootc, unsigned file_flags) {
  if (!fp)
    return -1;
  if (rootc < 0)
    return -1;
  (void)file_flags;
  thvec_t order;
  thvec_init(&order);
  for (lssize_t i = 0; i < rootc; i++) {
    int rc = collect_subset(&order, roots[i]);
    if (rc != 0) {
      thvec_free(&order);
      return rc;
    }
  }

  strpool_t sp;
  strpool_t yp;
  strpool_init(&sp);
  strpool_init(&yp);
  for (lssize_t i = 0; i < order.size; i++) {
    if (pool_collect_from_thunk(&sp, &yp, order.items[i]) != 0) {
      strpool_free(&sp);
      strpool_free(&yp);
      thvec_free(&order);
      return -1;
    }
  }

  uint32_t magic = LSTB_MAGIC;
  uint16_t vmaj = LSTB_VERSION_MAJOR, vmin = LSTB_VERSION_MINOR;
  uint32_t flags = file_flags;
  if (fwrite(&magic, sizeof(magic), 1, fp) != 1) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  if (fwrite(&vmaj, sizeof(vmaj), 1, fp) != 1) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  if (fwrite(&vmin, sizeof(vmin), 1, fp) != 1) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  if (fwrite(&flags, sizeof(flags), 1, fp) != 1) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  uint16_t scount = 5;
  if (fwrite(&scount, sizeof(scount), 1, fp) != 1) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }

  if (write_pool(fp, &sp) != 0) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  if (write_pool(fp, &yp) != 0) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  if (write_varuint(fp, 0) != 0) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }

  if (write_varuint(fp, (uint64_t)order.size) != 0) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  for (lssize_t i = 0; i < order.size; i++) {
    lsthunk_t* t = order.items[i];
    uint8_t    kind;
    switch (t->lt_type) {
    case LSTTYPE_INT:
      kind = LSTB_KIND_INT;
      break;
    case LSTTYPE_STR:
      kind = LSTB_KIND_STR;
      break;
    case LSTTYPE_SYMBOL:
      kind = LSTB_KIND_SYMBOL;
      break;
    case LSTTYPE_ALGE:
      kind = LSTB_KIND_ALGE;
      break;
    case LSTTYPE_BOTTOM:
      kind = LSTB_KIND_BOTTOM;
      break;
    default:
      thvec_free(&order);
      strpool_free(&sp);
      strpool_free(&yp);
      return -1;
    }
    if (fputc(kind, fp) == EOF) {
      thvec_free(&order);
      strpool_free(&sp);
      strpool_free(&yp);
      return -1;
    }
    uint8_t eflags = 0;
    if (t->lt_whnf == t)
      eflags |= LSTB_EF_WHNF;
    if (fputc(eflags, fp) == EOF) {
      thvec_free(&order);
      strpool_free(&sp);
      strpool_free(&yp);
      return -1;
    }
    if (write_varuint(fp, 0) != 0) {
      thvec_free(&order);
      strpool_free(&sp);
      strpool_free(&yp);
      return -1;
    }
    switch (t->lt_type) {
    case LSTTYPE_INT: {
      int64_t v = (int64_t)lsint_get(t->lt_int);
      if (write_varint(fp, v) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      break;
    }
    case LSTTYPE_STR: {
      lssize_t sid = strpool_index_of(&sp, lsstr_get_buf(t->lt_str));
      if (sid < 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (write_varuint(fp, (uint64_t)sid) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      break;
    }
    case LSTTYPE_SYMBOL: {
      lssize_t yid = strpool_index_of(&yp, lsstr_get_buf(t->lt_symbol));
      if (yid < 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (write_varuint(fp, (uint64_t)yid) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      break;
    }
    case LSTTYPE_ALGE: {
      lssize_t yid = strpool_index_of(&yp, lsstr_get_buf(t->lt_alge.lta_constr));
      if (yid < 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (write_varuint(fp, (uint64_t)yid) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (write_varuint(fp, (uint64_t)t->lt_alge.lta_argc) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      for (lssize_t j = 0; j < t->lt_alge.lta_argc; j++) {
        lssize_t id = thvec_index_of(&order, t->lt_alge.lta_args[j]);
        if (id < 0) {
          thvec_free(&order);
          strpool_free(&sp);
          strpool_free(&yp);
          return -1;
        }
        if (write_varuint(fp, (uint64_t)id) != 0) {
          thvec_free(&order);
          strpool_free(&sp);
          strpool_free(&yp);
          return -1;
        }
      }
      break;
    }
    case LSTTYPE_BOTTOM: {
      const char* m   = t->lt_bottom.lt_msg ? t->lt_bottom.lt_msg : "";
      lssize_t    sid = strpool_index_of(&sp, m);
      if (sid < 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (write_varuint(fp, (uint64_t)sid) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (fputc(0, fp) == EOF) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      if (write_varuint(fp, (uint64_t)t->lt_bottom.lt_rel.lbr_argc) != 0) {
        thvec_free(&order);
        strpool_free(&sp);
        strpool_free(&yp);
        return -1;
      }
      for (lssize_t j = 0; j < t->lt_bottom.lt_rel.lbr_argc; j++) {
        lssize_t id = thvec_index_of(&order, t->lt_bottom.lt_rel.lbr_args[j]);
        if (id < 0) {
          thvec_free(&order);
          strpool_free(&sp);
          strpool_free(&yp);
          return -1;
        }
        if (write_varuint(fp, (uint64_t)id) != 0) {
          thvec_free(&order);
          strpool_free(&sp);
          strpool_free(&yp);
          return -1;
        }
      }
      break;
    }
    default:
      break;
    }
  }

  if (write_varuint(fp, (uint64_t)rootc) != 0) {
    thvec_free(&order);
    strpool_free(&sp);
    strpool_free(&yp);
    return -1;
  }
  for (lssize_t i = 0; i < rootc; i++) {
    lssize_t id = thvec_index_of(&order, roots[i]);
    if (id < 0) {
      thvec_free(&order);
      strpool_free(&sp);
      strpool_free(&yp);
      return -1;
    }
    if (write_varuint(fp, (uint64_t)id) != 0) {
      thvec_free(&order);
      strpool_free(&sp);
      strpool_free(&yp);
      return -1;
    }
  }

  thvec_free(&order);
  strpool_free(&sp);
  strpool_free(&yp);
  return 0;
}

int lstb_read(FILE* fp, lsthunk_t*** out_roots, lssize_t* out_rootc, unsigned* out_file_flags,
              lstenv_t* prelude_env) {
  (void)prelude_env;
  if (!fp || !out_roots || !out_rootc || !out_file_flags)
    return -1;
  uint32_t magic = 0;
  uint16_t vmaj = 0, vmin = 0;
  uint32_t flags  = 0;
  uint16_t scount = 0;
  if (fread(&magic, sizeof(magic), 1, fp) != 1)
    return -1;
  if (magic != LSTB_MAGIC)
    return -1;
  if (fread(&vmaj, sizeof(vmaj), 1, fp) != 1)
    return -1;
  if (fread(&vmin, sizeof(vmin), 1, fp) != 1)
    return -1;
  if (fread(&flags, sizeof(flags), 1, fp) != 1)
    return -1;
  if (fread(&scount, sizeof(scount), 1, fp) != 1)
    return -1;
  if (vmaj != LSTB_VERSION_MAJOR)
    return -1;
  *out_file_flags = flags;
  (void)scount;

  lssize_t sp_cnt = 0;
  char**   sp     = read_pool(fp, &sp_cnt);
  if (sp_cnt < 0)
    return -1;
  lssize_t yp_cnt  = 0;
  char**   yp      = read_pool(fp, &yp_cnt);
  uint64_t pat_cnt = 0;
  if (read_varuint(fp, &pat_cnt) != 0) {
    for (lssize_t i = 0; i < sp_cnt; i++)
      free(sp[i]);
    free(sp);
    for (lssize_t i = 0; i < yp_cnt; i++)
      free(yp[i]);
    free(yp);
    return -1;
  }
  if (pat_cnt != 0) {
    for (lssize_t i = 0; i < sp_cnt; i++)
      free(sp[i]);
    free(sp);
    for (lssize_t i = 0; i < yp_cnt; i++)
      free(yp[i]);
    free(yp);
    return -1;
  }

  uint64_t tcount64 = 0;
  if (read_varuint(fp, &tcount64) != 0) {
    for (lssize_t i = 0; i < sp_cnt; i++)
      free(sp[i]);
    free(sp);
    for (lssize_t i = 0; i < yp_cnt; i++)
      free(yp[i]);
    free(yp);
    return -1;
  }
  lssize_t    tcount = (lssize_t)tcount64;
  lsthunk_t** nodes  = tcount ? (lsthunk_t**)lsmalloc(sizeof(lsthunk_t*) * (size_t)tcount) : NULL;
  typedef struct {
    lssize_t  argc;
    lssize_t* ids;
  } pend_alge_t;
  pend_alge_t* pend = tcount ? (pend_alge_t*)calloc((size_t)tcount, sizeof(pend_alge_t)) : NULL;
  if (tcount && (!nodes || !pend)) {
    for (lssize_t i = 0; i < sp_cnt; i++)
      free(sp[i]);
    free(sp);
    for (lssize_t i = 0; i < yp_cnt; i++)
      free(yp[i]);
    free(yp);
    free(nodes);
    free(pend);
    return -1;
  }

  for (lssize_t i = 0; i < tcount; i++) {
    int kch = fgetc(fp);
    if (kch == EOF)
      goto fail;
    uint8_t kind = (uint8_t)kch;
    int     ef   = fgetc(fp);
    if (ef == EOF)
      goto fail;
    (void)ef;
    uint64_t size_ign = 0;
    if (read_varuint(fp, &size_ign) != 0)
      goto fail;
    (void)size_ign;
    switch (kind) {
    case LSTB_KIND_INT: {
      int64_t v = 0;
      if (read_varint(fp, &v) != 0)
        goto fail;
      nodes[i] = lsthunk_new_int(lsint_new((int)v));
      break;
    }
    case LSTB_KIND_STR: {
      uint64_t sid = 0;
      if (read_varuint(fp, &sid) != 0)
        goto fail;
      if (sid >= (uint64_t)sp_cnt)
        goto fail;
      const lsstr_t* s = lsstr_new(sp[sid], (lssize_t)strlen(sp[sid]));
      nodes[i]         = lsthunk_new_str(s);
      break;
    }
    case LSTB_KIND_SYMBOL: {
      uint64_t yid = 0;
      if (read_varuint(fp, &yid) != 0)
        goto fail;
      if (yid >= (uint64_t)yp_cnt)
        goto fail;
      const lsstr_t* s = lsstr_new(yp[yid], (lssize_t)strlen(yp[yid]));
      nodes[i]         = lsthunk_new_symbol(s);
      break;
    }
    case LSTB_KIND_ALGE: {
      uint64_t yid = 0;
      if (read_varuint(fp, &yid) != 0)
        goto fail;
      if (yid >= (uint64_t)yp_cnt)
        goto fail;
      uint64_t argc64 = 0;
      if (read_varuint(fp, &argc64) != 0)
        goto fail;
      lssize_t   argc = (lssize_t)argc64;
      lsthunk_t* t    = lsmalloc(lssizeof(lsthunk_t, lt_alge) + (size_t)argc * sizeof(lsthunk_t*));
      t->lt_type      = LSTTYPE_ALGE;
      t->lt_whnf      = t;
      t->lt_trace_id  = g_trace_next_id++;
      t->lt_alge.lta_constr = lsstr_new(yp[yid], (lssize_t)strlen(yp[yid]));
      t->lt_alge.lta_argc   = argc;
      nodes[i]              = t;
      if (argc > 0) {
        pend[i].argc = argc;
        pend[i].ids  = (lssize_t*)malloc(sizeof(lssize_t) * (size_t)argc);
        if (!pend[i].ids)
          goto fail;
        for (lssize_t j = 0; j < argc; j++) {
          uint64_t idj = 0;
          if (read_varuint(fp, &idj) != 0)
            goto fail;
          pend[i].ids[j] = (lssize_t)idj;
        }
      }
      break;
    }
    case LSTB_KIND_BOTTOM: {
      uint64_t sid = 0;
      if (read_varuint(fp, &sid) != 0)
        goto fail;
      if (sid >= (uint64_t)sp_cnt)
        goto fail;
      int loc_mode = fgetc(fp);
      if (loc_mode == EOF)
        goto fail;
      (void)loc_mode;
      uint64_t rc64 = 0;
      if (read_varuint(fp, &rc64) != 0)
        goto fail;
      lssize_t  rc  = (lssize_t)rc64;
      lssize_t* ids = rc ? (lssize_t*)malloc(sizeof(lssize_t) * (size_t)rc) : NULL;
      if (rc && !ids)
        goto fail;
      for (lssize_t j = 0; j < rc; j++) {
        uint64_t idj = 0;
        if (read_varuint(fp, &idj) != 0) {
          free(ids);
          goto fail;
        }
        ids[j] = (lssize_t)idj;
      }
      lsthunk_t** args = rc ? (lsthunk_t**)malloc(sizeof(lsthunk_t*) * (size_t)rc) : NULL;
      if (rc && !args) {
        free(ids);
        goto fail;
      }
      for (lssize_t j = 0; j < rc; j++)
        args[j] = NULL;
      lsthunk_t* t = lsthunk_new_bottom(sp[sid], lstrace_take_pending_or_unknown(), rc, args);
      nodes[i]     = t;
      pend[i].argc = rc;
      pend[i].ids  = ids;
      free(args);
      break;
    }
    default:
      goto fail;
    }
    if (!nodes[i])
      goto fail;
  }

  for (lssize_t i = 0; i < tcount; i++) {
    if (pend[i].argc > 0) {
      if (nodes[i]->lt_type == LSTTYPE_ALGE) {
        for (lssize_t j = 0; j < pend[i].argc; j++)
          nodes[i]->lt_alge.lta_args[j] = nodes[pend[i].ids[j]];
      } else if (nodes[i]->lt_type == LSTTYPE_BOTTOM) {
        for (lssize_t j = 0; j < pend[i].argc; j++)
          nodes[i]->lt_bottom.lt_rel.lbr_args[j] = nodes[pend[i].ids[j]];
      }
      free(pend[i].ids);
      pend[i].ids  = NULL;
      pend[i].argc = 0;
    }
  }

  uint64_t rc64 = 0;
  if (read_varuint(fp, &rc64) != 0)
    goto fail;
  lssize_t    rootc = (lssize_t)rc64;
  lsthunk_t** roots = rootc ? (lsthunk_t**)lsmalloc(sizeof(lsthunk_t*) * (size_t)rootc) : NULL;
  for (lssize_t i = 0; i < rootc; i++) {
    uint64_t id = 0;
    if (read_varuint(fp, &id) != 0) {
      free(roots);
      goto fail;
    }
    if (id >= (uint64_t)tcount) {
      free(roots);
      goto fail;
    }
    roots[i] = nodes[id];
  }

  for (lssize_t i = 0; i < sp_cnt; i++)
    free(sp[i]);
  free(sp);
  for (lssize_t i = 0; i < yp_cnt; i++)
    free(yp[i]);
  free(yp);
  free(pend);
  free(nodes);
  *out_roots = roots;
  *out_rootc = rootc;
  return 0;

fail:
  if (sp) {
    for (lssize_t i = 0; i < sp_cnt; i++)
      free(sp[i]);
    free(sp);
  }
  if (yp) {
    for (lssize_t i = 0; i < yp_cnt; i++)
      free(yp[i]);
    free(yp);
  }
  if (pend) {
    for (lssize_t i = 0; i < tcount; i++)
      if (pend[i].ids)
        free(pend[i].ids);
    free(pend);
  }
  if (nodes)
    free(nodes);
  return -1;
}
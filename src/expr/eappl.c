#include "expr/eappl.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"
#include <string.h>

struct lseappl {
  const lsexpr_t* lea_func;
  lssize_t        lea_argc;
  const lsexpr_t* lea_args[0];
};

const lseappl_t* lseappl_new(const lsexpr_t* func, size_t argc, const lsexpr_t* args[]) {
  lseappl_t* eappl = lsmalloc(sizeof(lseappl_t) + argc * sizeof(lsexpr_t*));
  eappl->lea_func  = func;
  eappl->lea_argc  = (lssize_t)argc;
  for (lssize_t i = 0; i < (lssize_t)argc; i++)
    eappl->lea_args[i] = args[i];
  return eappl;
}

const lseappl_t* lseappl_add_arg(const lseappl_t* eappl, const lsexpr_t* arg) {
  lssize_t   argc      = eappl->lea_argc;
  lseappl_t* eappl_new = lsmalloc(sizeof(lseappl_t) + (argc + 1) * sizeof(lsexpr_t*));
  eappl_new->lea_func  = eappl->lea_func;
  eappl_new->lea_argc  = argc + 1;
  for (lssize_t i = 0; i < argc; i++)
    eappl_new->lea_args[i] = eappl->lea_args[i];
  eappl_new->lea_args[argc] = arg;
  return eappl_new;
}

const lseappl_t* lseappl_concat_args(lseappl_t* eappl, size_t argc, const lsexpr_t* args[]) {
  lssize_t   argc0     = eappl->lea_argc;
  lseappl_t* eappl_new = lsmalloc(sizeof(lseappl_t) + (argc0 + (lssize_t)argc) * sizeof(lsexpr_t*));
  eappl_new->lea_func  = eappl->lea_func;
  eappl_new->lea_argc  = argc0 + (lssize_t)argc;
  for (lssize_t i = 0; i < argc0; i++)
    eappl_new->lea_args[i] = eappl->lea_args[i];
  for (lssize_t i = 0; i < (lssize_t)argc; i++)
    eappl_new->lea_args[argc0 + i] = args[i];
  return eappl_new;
}

const lsexpr_t*        lseappl_get_func(const lseappl_t* eappl) { return eappl->lea_func; }

lssize_t               lseappl_get_argc(const lseappl_t* eappl) { return eappl->lea_argc; }

const lsexpr_t* const* lseappl_get_args(const lseappl_t* eappl) { return eappl->lea_args; }

static int is_dot_symbol_or_ctor(const lsexpr_t* e) {
  if (!e) return 0;
  lsexpr_type_query_t t = lsexpr_typeof(e);
  if (t == LSEQ_SYMBOL) return 1;
  if (t == LSEQ_ALGE) {
    const lsealge_t* a = lsexpr_get_alge(e);
    if (lsealge_get_argc(a) == 0) {
      const lsstr_t* cname = lsealge_get_constr(a);
      const char* cb = cname ? lsstr_get_buf(cname) : NULL;
      if (cb && cb[0] == '.') return 1;
    }
  }
  return 0;
}

void lseappl_print(FILE* stream, lsprec_t prec, int indent, const lseappl_t* eappl) {
  lssize_t argc = eappl->lea_argc;
  const lsexpr_t* func = eappl->lea_func;
  if (argc == 0) {
    lsexpr_print(stream, prec, indent, func);
    return;
  }
  if (lsfmt_is_active()) {
    lsfmt_flush_comments_up_to(stream, lsexpr_get_loc(func).first_line, indent);
  }

  if (prec > LSPREC_APPL) lsprintf(stream, indent, "(");

  // Print callee with minimal parentheses
  if (func && lsexpr_typeof(func) == LSEQ_APPL)
    lsexpr_print(stream, LSPREC_APPL, indent, func);
  else
    lsexpr_print(stream, LSPREC_APPL + 1, indent, func);

  int chain_active = 0; // currently emitting a dot chain
  int last_was_non_dot = 0; // last emitted token (after callee) was non-dot
  for (lssize_t i = 0; i < argc; i++) {
    const lsexpr_t* arg = eappl->lea_args[i];
    if (lsfmt_is_active()) {
      lsfmt_flush_comments_up_to(stream, lsexpr_get_loc(arg).first_line, indent);
    }

    int omit_space = 0;
    int is_dot = is_dot_symbol_or_ctor(arg);

    // Special case: first printed argument is an application (left .sym)
    if (i == 0 && arg && lsexpr_typeof(arg) == LSEQ_APPL) {
      const lseappl_t* inner = lsexpr_get_appl(arg);
      const lsexpr_t* in_left = lseappl_get_func(inner);
      if (lseappl_get_argc(inner) == 1) {
        const lsexpr_t* in_right = lseappl_get_args(inner)[0];
        if (is_dot_symbol_or_ctor(in_right)) {
          lsprintf(stream, indent, " ");
          lsexpr_print(stream, LSPREC_APPL + 1, indent, in_left);
          // fuse right without space
          lsexpr_print(stream, LSPREC_APPL + 1, indent, in_right);
          chain_active = 1;
          last_was_non_dot = 0;
          continue;
        }
      }
    }

    // Simple concatenation rule based on local state
    if (is_dot) {
      if (i == 0 || last_was_non_dot || chain_active) omit_space = 1;
    }

    if (!omit_space) lsprintf(stream, indent, " ");
    lsexpr_print(stream, LSPREC_APPL + 1, indent, arg);

    // update chain state
    if (is_dot) { chain_active = 1; last_was_non_dot = 0; }
    else { chain_active = 0; last_was_non_dot = 1; }
  }

  if (prec > LSPREC_APPL) lsprintf(stream, indent, ")");
}
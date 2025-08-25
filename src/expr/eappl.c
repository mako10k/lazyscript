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
  // allocate space for flexible array member lea_args[argc]
  lseappl_t* eappl = lsmalloc(sizeof(lseappl_t) + argc * sizeof(lsexpr_t*));
  eappl->lea_func  = func;
  eappl->lea_argc  = argc;
  for (lssize_t i = 0; i < argc; i++)
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
  lseappl_t* eappl_new = lsmalloc(sizeof(lseappl_t) + (argc0 + argc) * sizeof(lsexpr_t*));
  eappl_new->lea_func  = eappl->lea_func;
  eappl_new->lea_argc  = argc0 + argc;
  for (lssize_t i = 0; i < argc0; i++)
    eappl_new->lea_args[i] = eappl->lea_args[i];
  for (lssize_t i = 0; i < argc; i++)
    eappl_new->lea_args[argc0 + i] = args[i];
  return eappl_new;
}

const lsexpr_t*        lseappl_get_func(const lseappl_t* eappl) { return eappl->lea_func; }

lssize_t               lseappl_get_argc(const lseappl_t* eappl) { return eappl->lea_argc; }

const lsexpr_t* const* lseappl_get_args(const lseappl_t* eappl) { return eappl->lea_args; }

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
  // Pre-scan arguments to build grouping units: if a term and the next arg are tightly
  // adjacent in source (no whitespace), group them with parentheses when there was a
  // preceding loose application.
  typedef struct { int is_group; const lsexpr_t* left; const lsexpr_t* right; } unit_t;
  // Max args are small; allocate units on stack-sized array
  unit_t units[128]; size_t ucount = 0;
  // seed with callee as a non-group unit (stored in left)
  units[ucount++] = (unit_t){ .is_group = 0, .left = func, .right = NULL };
  for (size_t i = 0; i < (size_t)argc; i++) {
    const lsexpr_t* prev = units[ucount - 1].is_group ? NULL : units[ucount - 1].left;
    const lsexpr_t* arg  = eappl->lea_args[i];
    int make_group = 0;
    if (prev != NULL && ucount >= 2) {
      lsloc_t lp = lsexpr_get_loc(prev), la = lsexpr_get_loc(arg);
      const char* f1 = lp.filename, *f2 = la.filename;
      int same_file = (f1 == f2) || (f1 && f2 && strcmp(f1, f2) == 0);
    if (same_file && lp.last_line == la.first_line &&
      (la.first_column == lp.last_column || la.first_column == lp.last_column + 1)) {
        make_group = 1; // tight adjacency: bind stronger
      }
      // Fallback heuristic: group when current token is a dot symbol or a string literal
      if (!make_group) {
        lsexpr_type_query_t at = lsexpr_typeof(arg);
        if (at == LSEQ_SYMBOL || at == LSEQ_STR) make_group = 1;
      }
    }
    if (make_group) {
      // Replace the last unit with a grouped application of (prev arg)
      units[ucount - 1] = (unit_t){ .is_group = 1, .left = prev, .right = arg };
    } else {
      units[ucount++] = (unit_t){ .is_group = 0, .left = arg, .right = NULL };
    }
  }

  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, "(");

  // Print callee. If callee is an application, keep same precedence to avoid extra parens.
  if (func && lsexpr_typeof(func) == LSEQ_APPL)
    lsexpr_print(stream, LSPREC_APPL, indent, func);
  else
    lsexpr_print(stream, LSPREC_APPL + 1, indent, func);

  // Print remaining units
  for (size_t ui = 1; ui < ucount; ui++) {
    const unit_t* u = &units[ui];
    if (!u->is_group) {
      const lsexpr_t* arg = u->left;
      if (lsfmt_is_active()) {
        lsfmt_flush_comments_up_to(stream, lsexpr_get_loc(arg).first_line, indent);
      }
      int omit_space = 0;
      // For the very first argument, suppress space if it's a dot symbol or 0-ary '.' constructor
      if (ui == 1 && arg) {
        if (lsexpr_typeof(arg) == LSEQ_SYMBOL) {
          omit_space = 1;
        } else if (lsexpr_typeof(arg) == LSEQ_ALGE) {
          const lsealge_t* a = lsexpr_get_alge(arg);
          if (lsealge_get_argc(a) == 0) {
            const lsstr_t* cname = lsealge_get_constr(a);
            const char* cb = cname ? lsstr_get_buf(cname) : NULL;
            if (cb && cb[0] == '.') omit_space = 1;
          }
        }
      }
      if (!omit_space) lsprintf(stream, indent, " ");
      lsexpr_print(stream, LSPREC_APPL + 1, indent, arg);
    } else {
      // Grouped: print as " (left right)"
      if (lsfmt_is_active()) {
        // flush up to left first line to preserve comment order
        lsfmt_flush_comments_up_to(stream, lsexpr_get_loc(u->left).first_line, indent);
      }
      lsprintf(stream, indent, " (");
      lsexpr_print(stream, LSPREC_APPL + 1, indent, u->left);
      lsprintf(stream, indent, " ");
      lsexpr_print(stream, LSPREC_APPL + 1, indent, u->right);
      lsprintf(stream, indent, ")");
    }
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, ")");
}
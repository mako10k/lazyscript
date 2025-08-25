#include "expr/eappl.h"
#include "common/io.h"
#include "common/malloc.h"
#include "expr/expr.h"

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
  if (argc == 0) {
    lsexpr_print(stream, prec, indent, eappl->lea_func);
    return;
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, "(");
  // Print callee: if the callee itself is an application, keep the same precedence
  // to preserve left associativity without extra parentheses.
  const lsexpr_t* func = eappl->lea_func;
  if (func && lsexpr_typeof(func) == LSEQ_APPL)
    lsexpr_print(stream, LSPREC_APPL, indent, func);
  else
    lsexpr_print(stream, LSPREC_APPL + 1, indent, func);

  for (size_t i = 0; i < (size_t)argc; i++) {
    const lsexpr_t* arg = eappl->lea_args[i];
    int omit_space = 0;
    // If first argument is a dot symbol (or 0-ary alge starting with '.' for back-compat),
    // print it immediately after callee (e.g., ~internal.include)
    if (i == 0 && arg) {
      if (lsexpr_typeof(arg) == LSEQ_SYMBOL) {
        omit_space = 1;
      } else if (lsexpr_typeof(arg) == LSEQ_ALGE) {
        const lsealge_t* a = lsexpr_get_alge(arg);
        if (lsealge_get_argc(a) == 0) {
          const lsstr_t* cname = lsealge_get_constr(a);
          const char*    cb    = cname ? lsstr_get_buf(cname) : NULL;
          if (cb && cb[0] == '.') omit_space = 1;
        }
      }
    }
    if (!omit_space)
      lsprintf(stream, indent, " ");
    lsexpr_print(stream, LSPREC_APPL + 1, indent, arg);
  }
  if (prec > LSPREC_APPL)
    lsprintf(stream, indent, ")");
}
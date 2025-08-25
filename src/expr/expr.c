#include "expr/expr.h"
#include "common/io.h"
#include "common/loc.h"
#include "common/malloc.h"
#include "common/ref.h"
#include "expr/ealge.h"
#include "expr/eappl.h"
#include "expr/echoice.h"
#include "expr/eclosure.h"
#include "expr/elambda.h"
#include "expr/enslit.h"
#include <assert.h>
#include "misc/prog.h"
#include <stdlib.h>
#include <string.h>
#include "pat/pat.h"

// --- Do-notation re-sugaring for formatter ---
// Detect expressions shaped like:
//   ((~ns return) x)
//   ((~ns bind)   A (\p -> rest))
//   ((~ns chain)  A (\_ -> rest))
// and print them as:
//   !{ x }
//   !{ ~p <- A; ... }
//   !{ A; ... }
// Only active when lsfmt_is_active() is true (formatter mode).

static int is_zero_ary_ctor_named(const lsexpr_t* e, const char* name) {
  if (!e || lsexpr_typeof(e) != LSEQ_ALGE) return 0;
  const lsealge_t* a = lsexpr_get_alge(e);
  if (lsealge_get_argc(a) != 0) return 0;
  const lsstr_t* cn = lsealge_get_constr(a);
  const char*    cs = cn ? lsstr_get_buf(cn) : NULL;
  return cs && strcmp(cs, name) == 0;
}

static int is_prelude_op(const lsexpr_t* e, const char* opname) {
  // e should be application: ((~ns opname) ...)
  if (!e || lsexpr_typeof(e) != LSEQ_APPL) return 0;
  const lseappl_t* ap = lsexpr_get_appl(e);
  if (!ap || lseappl_get_argc(ap) < 1) return 0;
  const lsexpr_t* f = lseappl_get_func(ap);
  if (!f || lsexpr_typeof(f) != LSEQ_APPL) return 0;
  const lseappl_t* fap = lsexpr_get_appl(f);
  if (!fap || lseappl_get_argc(fap) != 1) return 0;
  const lsexpr_t* arg0 = lseappl_get_args(fap)[0];
  return is_zero_ary_ctor_named(arg0, opname);
}

static int match_return_call(const lsexpr_t* e, const lsexpr_t** out_x) {
  if (!is_prelude_op(e, "return")) return 0;
  const lseappl_t* ap = lsexpr_get_appl(e);
  if (lseappl_get_argc(ap) != 1) return 0;
  if (out_x) *out_x = lseappl_get_args(ap)[0];
  return 1;
}

static int match_chain_call(const lsexpr_t* e, const lsexpr_t** out_a, const lselambda_t** out_lam) {
  if (!is_prelude_op(e, "chain")) return 0;
  const lseappl_t* ap = lsexpr_get_appl(e);
  if (lseappl_get_argc(ap) != 2) return 0;
  const lsexpr_t* a   = lseappl_get_args(ap)[0];
  const lsexpr_t* lam = lseappl_get_args(ap)[1];
  if (!lam || lsexpr_typeof(lam) != LSEQ_LAMBDA) return 0;
  const lselambda_t* l = lsexpr_get_lambda(lam);
  const lspat_t*     p = lselambda_get_param(l);
  if (!p || lspat_get_type(p) != LSPTYPE_WILDCARD) return 0; // must be wildcard
  if (out_a) *out_a = a;
  if (out_lam) *out_lam = l;
  return 1;
}

static int match_bind_call(const lsexpr_t* e, const lsexpr_t** out_a, const lselambda_t** out_lam) {
  if (!is_prelude_op(e, "bind")) return 0;
  const lseappl_t* ap = lsexpr_get_appl(e);
  if (lseappl_get_argc(ap) != 2) return 0;
  const lsexpr_t* a   = lseappl_get_args(ap)[0];
  const lsexpr_t* lam = lseappl_get_args(ap)[1];
  if (!lam || lsexpr_typeof(lam) != LSEQ_LAMBDA) return 0;
  const lselambda_t* l = lsexpr_get_lambda(lam);
  const lspat_t*     p = lselambda_get_param(l);
  if (!p || lspat_get_type(p) != LSPTYPE_REF) return 0; // must be ref pattern
  if (out_a) *out_a = a;
  if (out_lam) *out_lam = l;
  return 1;
}

// Helper: is expression (~prelude return) ()
static int is_return_unit(const lsexpr_t* e) {
  const lsexpr_t* rx = NULL;
  if (!match_return_call(e, &rx)) return 0;
  if (!rx || lsexpr_typeof(rx) != LSEQ_ALGE) return 0;
  const lsealge_t* ua = lsexpr_get_alge(rx);
  if (lsealge_get_argc(ua) != 0) return 0;
  const lsstr_t* cn = lsealge_get_constr(ua);
  const char*    cs = cn ? lsstr_get_buf(cn) : NULL;
  return cs && strcmp(cs, ",") == 0;
}

// Pretty-print do-body as one statement per line at given indent.
// Each printed statement ends with ";\n". Suppresses final "return ()" entirely.
static int print_do_lines(FILE* fp, int indent, const lsexpr_t* e) {
  // If formatter is active, allow pending comments before this statement
  if (lsfmt_is_active() && e) {
    int line = lsexpr_get_loc(e).first_line;
    if (line > 0) lsfmt_flush_comments_up_to(fp, line - 1, indent);
  }
  // return x
  const lsexpr_t* rx = NULL;
  if (match_return_call(e, &rx)) {
    // skip unit
    if (!(rx && lsexpr_typeof(rx) == LSEQ_ALGE && is_zero_ary_ctor_named(rx, ","))) {
  lsexpr_print(fp, LSPREC_LOWEST, indent, rx);
  // Last statement in block: no indent seeding after newline
  lsprintf(fp, 0, ";\n");
    }
    return 1;
  }

  // chain A (_ -> rest)
  const lsexpr_t* a = NULL; const lselambda_t* lam = NULL;
  if (match_chain_call(e, &a, &lam)) {
    const lsexpr_t* rest = lselambda_get_body(lam);
  lsexpr_print(fp, LSPREC_LOWEST, indent, a);
    if (!is_return_unit(rest)) {
      // Seed indentation for next line
      lsprintf(fp, indent, ";\n");
      return print_do_lines(fp, indent, rest);
    } else {
      // Last statement
      lsprintf(fp, 0, ";\n");
    }
    return 1;
  }

  // bind A (x -> rest)
  if (match_bind_call(e, &a, &lam)) {
    const lspat_t* p = lselambda_get_param(lam);
    const lsexpr_t* rest = lselambda_get_body(lam);
    const lsexpr_t* rx2 = NULL;
    if (match_return_call(rest, &rx2)) {
      // tail-bind to value
      lsexpr_print(fp, LSPREC_LOWEST, indent, a);
      // Last statement
      lsprintf(fp, 0, ";\n");
      return 1;
    }
    // x <- A
    if (p && lspat_get_type(p) == LSPTYPE_REF) {
      const lsref_t* r = lspat_get_ref(p);
      const lsstr_t* rn = r ? lsref_get_name(r) : NULL;
      const char*    rb = rn ? lsstr_get_buf(rn) : "_";
      lsprintf(fp, indent, "%s", rb);
    } else {
      lspat_print(fp, LSPREC_LOWEST, indent, p);
    }
    lsprintf(fp, indent, " <- ");
    lsexpr_print(fp, LSPREC_LOWEST, indent, a);
    // Seed indentation for next line
    lsprintf(fp, indent, ";\n");
    return print_do_lines(fp, indent, rest);
  }

  return 0;
}

// Recursively print do-body statements from e. Returns 1 if fully handled.
// top_level: 1 when called for the whole body; affects unit suppression.
static int print_do_body(FILE* fp, int indent, const lsexpr_t* e, int top_level) {
  // Case 1: return x  => final expression x
  const lsexpr_t* rx = NULL;
  if (match_return_call(e, &rx)) {
    // Suppress printing of unit '()' anywhere (pretty-do); empty do handled elsewhere.
    if (!(rx && lsexpr_typeof(rx) == LSEQ_ALGE && is_zero_ary_ctor_named(rx, ","))) {
      lsexpr_print(fp, LSPREC_LOWEST, indent, rx);
    }
    return 1;
  }

  // Case 2: chain A (\_ -> rest) => print A; rest
  const lsexpr_t* a = NULL; const lselambda_t* lam = NULL;
  if (match_chain_call(e, &a, &lam)) {
    const lsexpr_t* rest = lselambda_get_body(lam);
    lsexpr_print(fp, LSPREC_LOWEST, indent, a);
    if (!is_return_unit(rest)) {
      lsprintf(fp, indent, "; ");
      return print_do_body(fp, indent, rest, 0);
    }
    return 1;
  }

  // Case 3: bind A (\x -> rest)
  if (match_bind_call(e, &a, &lam)) {
    const lspat_t* p = lselambda_get_param(lam);
    const lsexpr_t* rest = lselambda_get_body(lam);
    // Tail bind optimization: rest == return x
    const lsexpr_t* rx2 = NULL;
    if (match_return_call(rest, &rx2)) {
      // Print just A (final expression), avoid introducing a new binder syntactically.
      lsexpr_print(fp, LSPREC_LOWEST, indent, a);
      return 1;
    }
    // General bind: x <- A; rest
    // In do-notation, print ref pattern without leading '~'
    if (p && lspat_get_type(p) == LSPTYPE_REF) {
      const lsref_t* r = lspat_get_ref(p);
      const lsstr_t* rn = r ? lsref_get_name(r) : NULL;
      const char*    rb = rn ? lsstr_get_buf(rn) : "_";
      lsprintf(fp, indent, "%s", rb);
    } else {
      lspat_print(fp, LSPREC_LOWEST, indent, p);
    }
    lsprintf(fp, indent, " <- ");
    lsexpr_print(fp, LSPREC_LOWEST, indent, a);
    lsprintf(fp, indent, "; ");
    return print_do_body(fp, indent, rest, 0);
  }

  return 0; // not a recognized do-body
}

static int try_print_do_block(FILE* fp, lsprec_t prec, int indent, const lsexpr_t* e) {
  const int dbg = getenv("LAZYFMT_DEBUG_DO") && getenv("LAZYFMT_DEBUG_DO")[0] ? 1 : 0;
  // Fast check: top is bind/chain/return composed form
  if (!e || lsexpr_typeof(e) != LSEQ_APPL) {
    if (dbg) fprintf(stderr, "[do] top not appl: type=%d\n", e ? (int)lsexpr_typeof(e) : -1);
    // Allow empty do: ((~ns return) ()) rarely occurs standalone; skip here.
    return 0;
  }
  int is_do_shape = is_prelude_op(e, "bind") || is_prelude_op(e, "chain") || is_prelude_op(e, "return");
  if (dbg) fprintf(stderr, "[do] is_do_shape=%d\n", is_do_shape);
  if (!is_do_shape) return 0;

  // Special-case empty do: return ()
  const lsexpr_t* rx0 = NULL;
  if (match_return_call(e, &rx0)) {
    // Check unit () constructor: zero-ary ","
    if (rx0 && lsexpr_typeof(rx0) == LSEQ_ALGE) {
      const lsealge_t* ua = lsexpr_get_alge(rx0);
      if (lsealge_get_argc(ua) == 0) {
        const lsstr_t* cn = lsealge_get_constr(ua);
        const char*    cs = cn ? lsstr_get_buf(cn) : NULL;
        if (cs && strcmp(cs, ",") == 0) { lsprintf(fp, indent, "!{ }"); return 1; }
      }
    }
  }

  // Multiline pretty form
  if (dbg) fprintf(stderr, "[do] printing do-block (multiline)\n");
  // Open brace and seed inner indentation for the first body line
  lsprintf(fp, indent, "!{");
  lsprintf(fp, indent + 1, "\n");
  int ok = print_do_lines(fp, indent + 1, e);
  if (!ok) {
    if (dbg) fprintf(stderr, "[do] print_do_lines failed (not recognized)\n");
    return 0;
  }
  // Close at outer indent on its own line (avoid residual inner indent spaces)
  for (int sp = 0; sp < indent; sp++) lsprintf(fp, 0, "  ");
  lsprintf(fp, 0, "}");
  return 1;
}

struct lsexpr {
  lsetype_t le_type;
  lsloc_t   le_loc;
  union {
    const lsealge_t*    le_alge;
    const lseappl_t*    le_appl;
    const lsref_t*      le_ref;
    const lselambda_t*  le_lambda;
    const lseclosure_t* le_closure;
    const lsechoice_t*  le_choice;
    const lsint_t*      le_intval;
    const lsstr_t*      le_strval;
  const lsenslit_t*   le_nslit;
  const lsstr_t*      le_symbol; // dot symbol literal
  };
};

const lsexpr_t* lsexpr_new_alge(const lsealge_t* ealge) {
  lsexpr_t* expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type  = LSETYPE_ALGE;
    expr->le_loc   = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_alge  = ealge;
  return expr;
}

const lsexpr_t* lsexpr_new_nslit(const lsenslit_t* ens) {
  lsexpr_t* expr   = lsmalloc(sizeof(lsexpr_t));
  expr->le_type    = LSETYPE_NSLIT;
  expr->le_loc     = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_nslit   = ens;
  return expr;
}
const lsexpr_t* lsexpr_new_appl(const lseappl_t* eappl) {
  lsexpr_t* expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type  = LSETYPE_APPL;
  expr->le_loc   = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_appl  = eappl;
  return expr;
}

const lsexpr_t* lsexpr_new_ref(const lsref_t* ref) {
  lsexpr_t* expr = lsmalloc(sizeof(lsexpr_t));
  expr->le_type  = LSETYPE_REF;
  expr->le_loc   = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_ref   = ref;
  return expr;
}

const lsexpr_t* lsexpr_new_int(const lsint_t* intval) {
  lsexpr_t* expr  = lsmalloc(sizeof(lsexpr_t));
  expr->le_type   = LSETYPE_INT;
  expr->le_loc    = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_intval = intval;
  return expr;
}

const lsexpr_t* lsexpr_new_str(const lsstr_t* strval) {
  lsexpr_t* expr  = lsmalloc(sizeof(lsexpr_t));
  expr->le_type   = LSETYPE_STR;
  expr->le_loc    = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_strval = strval;
  return expr;
}

const lsexpr_t* lsexpr_new_lambda(const lselambda_t* lambda) {
  lsexpr_t* expr  = lsmalloc(sizeof(lsexpr_t));
  expr->le_type   = LSETYPE_LAMBDA;
  expr->le_loc    = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_lambda = lambda;
  return expr;
}

const lsexpr_t* lsexpr_new_choice(const lsechoice_t* echoice) {
  lsexpr_t* expr  = lsmalloc(sizeof(lsexpr_t));
  expr->le_type   = LSETYPE_CHOICE;
  expr->le_loc    = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_choice = echoice;
  return expr;
}

const lsexpr_t* lsexpr_new_symbol(const lsstr_t* sym) {
  lsexpr_t* expr  = lsmalloc(sizeof(lsexpr_t));
  expr->le_type   = LSETYPE_SYMBOL;
  expr->le_loc    = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_symbol = sym;
  return expr;
}

const lsexpr_t* lsexpr_new_closure(const lseclosure_t* closure) {
  lsexpr_t* expr   = lsmalloc(sizeof(lsexpr_t));
  expr->le_type    = LSETYPE_CLOSURE;
  expr->le_loc     = lsloc("<unknown>", 1, 1, 1, 1);
  expr->le_closure = closure;
  return expr;
}

lsetype_t        lsexpr_get_type(const lsexpr_t* expr) { return expr->le_type; }

const lsealge_t* lsexpr_get_alge(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_ALGE);
  return expr->le_alge;
}

const lseappl_t* lsexpr_get_appl(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_APPL);
  return expr->le_appl;
}

const lsref_t* lsexpr_get_ref(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_REF);
  return expr->le_ref;
}

const lsint_t* lsexpr_get_int(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_INT);
  return expr->le_intval;
}

const lsstr_t* lsexpr_get_str(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_STR);
  return expr->le_strval;
}

const lselambda_t* lsexpr_get_lambda(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_LAMBDA);
  return expr->le_lambda;
}

const lseclosure_t* lsexpr_get_closure(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_CLOSURE);
  return expr->le_closure;
}

const lsechoice_t* lsexpr_get_choice(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_CHOICE);
  return expr->le_choice;
}

void lsexpr_print(FILE* fp, lsprec_t prec, int indent, const lsexpr_t* expr) {
  const int dbg_do = getenv("LAZYFMT_DEBUG_DO") && getenv("LAZYFMT_DEBUG_DO")[0] ? 1 : 0;
  if (dbg_do) {
    fprintf(stderr, "[do] enter lsexpr_print: type=%d line=%d active=%d\n", (int)expr->le_type, expr->le_loc.first_line, lsfmt_is_active());
  }
  // Emit pending comments, with special handling for NSLIT to keep same-line EOL with '{' inside.
  if (lsfmt_is_active()) {
    if (expr->le_type == LSETYPE_NSLIT) {
      int line = expr->le_loc.first_line;
      if (line > 0) lsfmt_flush_comments_up_to(fp, line - 1, indent);
    } else {
      lsfmt_flush_comments_up_to(fp, expr->le_loc.first_line, indent);
    }
  }
  // Re-sugar do-notation when formatting if the expression matches bind/chain/return shapes.
  if (lsfmt_is_active()) {
    if (try_print_do_block(fp, prec, indent, expr)) return;
  }
  switch (expr->le_type) {
  case LSETYPE_ALGE:
    lsealge_print(fp, prec, indent, expr->le_alge);
    return;
  case LSETYPE_APPL:
    lseappl_print(fp, prec, indent, expr->le_appl);
    return;
  case LSETYPE_REF:
    lsref_print(fp, prec, indent, expr->le_ref);
    return;
  case LSETYPE_INT:
    lsint_print(fp, prec, indent, expr->le_intval);
    return;
  case LSETYPE_STR:
    lsstr_print(fp, prec, indent, expr->le_strval);
    return;
  case LSETYPE_LAMBDA:
    lselambda_print(fp, prec, indent, expr->le_lambda);
    return;
  case LSETYPE_CLOSURE:
    lseclosure_print(fp, prec, indent, expr->le_closure);
    return;
  case LSETYPE_CHOICE:
    lsechoice_print(fp, prec, indent, expr->le_choice);
    return;
  case LSETYPE_NSLIT:
    lsenslit_print(fp, prec, indent, expr->le_nslit, expr->le_loc);
    return;
  case LSETYPE_SYMBOL:
  // print symbol as bare (no quotes), literal includes the leading dot
  lsstr_print_bare(fp, prec, indent, expr->le_symbol);
    return;
  }
  lsprintf(fp, indent, "Unknown expression type %d\n", expr->le_type);
}

lsloc_t lsexpr_get_loc(const lsexpr_t* expr) {
  return expr->le_loc;
}

const lsenslit_t* lsexpr_get_nslit(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_NSLIT);
  return expr->le_nslit;
}

const lsstr_t* lsexpr_get_symbol(const lsexpr_t* expr) {
  assert(expr->le_type == LSETYPE_SYMBOL);
  return expr->le_symbol;
}
const lsexpr_t* lsexpr_with_loc(const lsexpr_t* expr_in, lsloc_t loc) {
  // cast away const for internal mutation; API returns same pointer as const
  lsexpr_t* expr = (lsexpr_t*)expr_in;
  expr->le_loc = loc;
  return expr_in;
}

lsexpr_type_query_t lsexpr_typeof(const lsexpr_t* expr) {
  return (lsexpr_type_query_t)expr->le_type;
}

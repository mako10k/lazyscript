#include "expr/enslit.h"
#include "common/io.h"
#include "common/malloc.h"
#include "common/str.h"
#include "misc/prog.h"
#include <limits.h>

struct lsenslit {
  lssize_t         count;
  const lsstr_t**  names; // length=count
  const lsexpr_t** exprs; // length=count
};

const lsenslit_t* lsenslit_new(lssize_t n, const lsstr_t* const* names,
                               const lsexpr_t* const* exprs) {
  lsenslit_t* ns = lsmalloc(sizeof(lsenslit_t));
  // lssize_t is unsigned in this project; assign directly to avoid -Wtype-limits.
  ns->count = n;
  if (ns->count == 0) {
    ns->names = NULL;
    ns->exprs = NULL;
    return ns;
  }
  ns->names = lsmalloc(sizeof(lsstr_t*) * (size_t)ns->count);
  ns->exprs = lsmalloc(sizeof(lsexpr_t*) * (size_t)ns->count);
  for (lssize_t i = 0; i < ns->count; i++) {
    ns->names[i] = names[i];
    ns->exprs[i] = exprs[i];
  }
  return ns;
}

lssize_t        lsenslit_get_count(const lsenslit_t* ns) { return ns->count; }
const lsstr_t*  lsenslit_get_name(const lsenslit_t* ns, lssize_t i) { return ns->names[i]; }
const lsexpr_t* lsenslit_get_expr(const lsenslit_t* ns, lssize_t i) { return ns->exprs[i]; }

void lsenslit_print(FILE* fp, lsprec_t prec, int indent, const lsenslit_t* ns, lsloc_t loc) {
  (void)prec;
  lssize_t n = ns->count;
  if (n == 0) {
    // Empty namespace literal in compact form
    lsprintf(fp, indent, "{ }");
    return;
  }

  // Multi-line pretty print
  lsprintf(fp, indent, "{");
  // newline right after '{'
  lsprintf(fp, indent + 1, "\n");
  // Place leading comment that conceptually belongs to the inside of the block.
  if (lsfmt_is_active() && n > 0) {
    int min_line = 0;
    for (lssize_t i = 0; i < n; i++) {
      int ln = lsexpr_get_loc(ns->exprs[i]).first_line;
      if (ln > 0 && (min_line == 0 || ln < min_line))
        min_line = ln;
    }
    if (min_line > 0) {
      // Emit exactly up to the first member's line-1 so the first member stays together
      lsfmt_flush_comments_up_to(fp, min_line - 1, indent + 1);
    }
  }
  for (lssize_t i = 0; i < n; i++) {
    // Flush comments up to this member's first line (use RHS/body loc)
    if (lsfmt_is_active()) {
      const lsexpr_t* rhs0 = ns->exprs[i];
      int             rl   = lsexpr_get_loc(rhs0).first_line;
      if (rl > 0)
        lsfmt_flush_comments_up_to(fp, rl - 1, indent + 1);
    }
    // Print name with a single leading '.'; avoid duplicating if already present
    const lsstr_t* name = ns->names[i];
    const char*    nb   = name ? lsstr_get_buf(name) : NULL;
    if (!(nb && nb[0] == '.'))
      lsprintf(fp, 0, ".");
    lsstr_print_bare(fp, LSPREC_LOWEST, 0, name);
    // If RHS is a lambda-chain, print in sugared form: .sym p1 p2 = body
    const lsexpr_t* rhs = ns->exprs[i];
    if (lsexpr_typeof(rhs) == LSEQ_LAMBDA) {
      const lsexpr_t* body = rhs;
      const lspat_t*  params[64];
      int             pc = 0;
      while (lsexpr_typeof(body) == LSEQ_LAMBDA && pc < 64) {
        const lselambda_t* lam = lsexpr_get_lambda(body);
        params[pc++]           = lselambda_get_param(lam);
        body                   = lselambda_get_body(lam);
      }
      if (lsfmt_is_active()) {
        lsfmt_flush_comments_up_to(fp, lsexpr_get_loc(body).first_line, indent + 1);
      }
      for (int j = 0; j < pc; j++) {
        lsprintf(fp, 0, " ");
        lspat_print(fp, LSPREC_APPL + 1, indent + 1, params[j]);
      }
      lsprintf(fp, 0, " = ");
      // Prevent nested printers from consuming same-line EOL comment intended for this member
      if (lsfmt_is_active())
        lsfmt_set_hold_line(lsexpr_get_loc(body).first_line);
      lsexpr_print(fp, LSPREC_LOWEST, indent + 1, body);
      if (lsfmt_is_active())
        lsfmt_clear_hold_line();
    } else {
      lsprintf(fp, 0, " = ");
      // Print expression; allow nested printers to indent relative to current level
      if (lsfmt_is_active())
        lsfmt_set_hold_line(lsexpr_get_loc(rhs).first_line);
      lsexpr_print(fp, LSPREC_LOWEST, indent + 1, rhs);
      if (lsfmt_is_active())
        lsfmt_clear_hold_line();
    }
    // Same-line EOL comment support: if the next pending comment is on the same
    // source line as the rhs/body we just printed, emit it at end-of-line.
    if (lsfmt_is_active()) {
      const lscomment_t* nc       = lsfmt_peek_next_comment();
      int                rhs_line = lsexpr_get_loc((lsexpr_typeof(rhs) == LSEQ_LAMBDA)
                                                       ? lselambda_get_body(lsexpr_get_lambda(rhs))
                                                       : rhs)
                         .first_line;
      if (nc && nc->lc_loc.first_line == rhs_line && nc->lc_text) {
        const char* s = lsstr_get_buf(nc->lc_text);
        if (s && s[0] == '#') {
          // We want ';' to terminate the member before the EOL comment, and avoid leaving trailing
          // indent spaces
          lsprintf(fp, 0, "; %s\n", s);
          lsfmt_consume_next_comment();
          // ';' and newline were already printed above, so skip printing another here.
          if (i + 1 == n) {
            // For last entry, after ending the line, flush any remaining block-internal comments
            if (lsfmt_is_active()) {
              const lscomment_t* nc2 = lsfmt_peek_next_comment();
              while (nc2 &&
                     nc2->lc_loc.first_line <= (loc.last_line > 0 ? loc.last_line - 1 : INT_MAX)) {
                if (nc2->lc_text) {
                  const char* s2 = lsstr_get_buf(nc2->lc_text);
                  if (s2 && s2[0])
                    lsprintf(fp, indent + 1, "%s\n", s2);
                }
                lsfmt_consume_next_comment();
                nc2 = lsfmt_peek_next_comment();
              }
            }
          } else {
            // Not last entry: seed indentation for the following standalone comments/member line.
            // NOTE: Avoid lsprintf with an empty string because it can clear the line-start state
            // and prevent subsequent flush from applying its own indentation. Instead, emit the
            // exact indentation spaces here so following comment lines start at the inner indent.
            for (int sp = 0; sp < indent + 1; sp++)
              lsprintf(fp, 0, "  ");
          }
          continue; // go to next member or close block
        }
      }
    }
    // No EOL comment: end the member cleanly with semicolon and newline
    if (i + 1 == n) {
      // Last member: avoid leaving trailing indent spaces for the next line (closing brace)
      lsprintf(fp, 0, ";\n");
      // Last entry: after ending the line, flush block-internal comments (before closing brace)
      if (lsfmt_is_active()) {
        const lscomment_t* nc = lsfmt_peek_next_comment();
        while (nc && nc->lc_loc.first_line <= (loc.last_line > 0 ? loc.last_line - 1 : INT_MAX)) {
          if (nc->lc_text) {
            const char* s = lsstr_get_buf(nc->lc_text);
            if (s && s[0]) {
              // Manually indent comment lines without causing trailing spaces after newline
              for (int sp = 0; sp < indent + 1; sp++)
                lsprintf(fp, 0, "  ");
              lsprintf(fp, 0, "%s\n", s);
            }
          }
          lsfmt_consume_next_comment();
          nc = lsfmt_peek_next_comment();
        }
      }
    } else {
      // Not last member: keep standard behavior which seeds indentation for the next line
      lsprintf(fp, indent + 1, ";\n");
    }
  }
  // Emit any comments that fall on the same line as the closing '}' or just before it
  if (lsfmt_is_active() && loc.last_line > 0) {
    lsfmt_flush_comments_up_to(fp, loc.last_line - 1, indent + 1);
  }
  // Print closing brace aligned to current indent without relying on lsprintf's post-newline
  // padding
  for (int sp = 0; sp < indent; sp++)
    lsprintf(fp, 0, "  ");
  lsprintf(fp, 0, "}");
}

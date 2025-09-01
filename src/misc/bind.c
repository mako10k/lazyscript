#include "misc/bind.h"
#include "common/io.h"
#include "common/malloc.h"
#include "misc/prog.h"
#include <assert.h>

struct lsbind {
  const lspat_t*  lbe_lhs;
  const lsexpr_t* lbe_rhs;
};

const lsbind_t* lsbind_new(const lspat_t* lhs, const lsexpr_t* rhs) {
  lsbind_t* bind = lsmalloc(sizeof(lsbind_t));
  bind->lbe_lhs  = lhs;
  bind->lbe_rhs  = rhs;
#if defined(DEBUG) && defined(DEBUG_VERBOSE)
  lsprintf(stderr, 0, "D: lsbind_new: ");
  lsbind_print(stderr, 0, 0, bind);
  lsprintf(stderr, 0, "\n");
#endif
  return bind;
}

void lsbind_print(FILE* fp, lsprec_t prec, int indent, const lsbind_t* bind) {
  // Emit comments that belong to this bind line before printing 'lhs ='
  if (lsfmt_is_active()) {
    int line = lsexpr_get_loc(bind->lbe_rhs).first_line;
    if (line > 0)
      lsfmt_flush_comments_up_to(fp, line - 1, indent);
    // If the next pending comment is on the same line as RHS, print it BEFORE the bind
    const lscomment_t* nc = lsfmt_peek_next_comment();
    if (nc && nc->lc_loc.first_line == line && nc->lc_text) {
      const char* s = lsstr_get_buf(nc->lc_text);
      if (s && s[0]) {
        lsprintf(fp, indent, "%s\n", s);
      }
      lsfmt_consume_next_comment();
    }
  }
  lspat_print(fp, prec, indent, bind->lbe_lhs);
  lsprintf(fp, indent, " = ");
  lsexpr_print(fp, prec, indent, bind->lbe_rhs);
}

const lspat_t*  lsbind_get_lhs(const lsbind_t* bind) { return bind->lbe_lhs; }

const lsexpr_t* lsbind_get_rhs(const lsbind_t* bind) { return bind->lbe_rhs; }
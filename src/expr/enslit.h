#pragma once

typedef struct lsenslit lsenslit_t;

#include "expr/expr.h"
#include "common/array.h"

// Immutable namespace literal (nslit) AST node
// Holds N entries: name[i] = expr[i]
lsapi_wur const lsenslit_t* lsenslit_new(lssize_t n, const lsstr_t* const* names,
                                         const lsexpr_t* const* exprs);

lsapi_get lssize_t                        lsenslit_get_count(const lsenslit_t* ns);
lsapi_get const lsstr_t*                  lsenslit_get_name(const lsenslit_t* ns, lssize_t i);
lsapi_get const lsexpr_t*                 lsenslit_get_expr(const lsenslit_t* ns, lssize_t i);

// Printing
lsapi_print void lsenslit_print(FILE* fp, lsprec_t prec, int indent, const lsenslit_t* ns);

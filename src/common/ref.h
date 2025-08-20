#pragma once

typedef struct lsref lsref_t;

#include "common/loc.h"
#include "common/str.h"

const lsref_t* lsref_new(const lsstr_t* name, lsloc_t loc);
const lsstr_t* lsref_get_name(const lsref_t* ref);
lsloc_t        lsref_get_loc(const lsref_t* ref);
void           lsref_print(FILE* fp, lsprec_t prec, int indent, const lsref_t* ref);
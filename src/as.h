#pragma once

typedef struct lsas lsas_t;

#include "pat.h"
#include "str.h"

lsas_t *lsas(const lsstr_t *name, const lspat_t *pat);
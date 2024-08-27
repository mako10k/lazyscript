#pragma once

typedef enum lsprec {
  LSPREC_LOWEST = 0,
  LSPREC_CONS,
  LSPREC_LAMBDA,
  LSPREC_APPL,
} lsprec_t;

typedef enum lspres {
  LSPRES_SUCCESS = 0,
  LSPRES_FAILURE,
} lspres_t;

typedef enum lsmres {
  LSMATCH_SUCCESS = 0,
  LSMATCH_FAILURE,
} lsmres_t;

typedef unsigned int lssize_t;
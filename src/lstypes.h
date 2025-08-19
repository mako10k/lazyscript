#pragma once

typedef enum lsprec {
  LSPREC_LOWEST,
  LSPREC_CHOICE,
  LSPREC_CONS,
  LSPREC_LAMBDA,
  LSPREC_APPL,
  LSPREC_HIGHEST,
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

#define lsapi_nn1 __attribute__((nonnull(1)))
#define lsapi_nn12 __attribute__((nonnull(1, 2)))
#define lsapi_nn13 __attribute__((nonnull(1, 3)))
#define lsapi_nn14 __attribute__((nonnull(1, 4)))
#define lsapi_nn2 __attribute__((nonnull(2)))
#define lsapi_nn23 __attribute__((nonnull(2, 3)))
#define lsapi_nn24 __attribute__((nonnull(2, 4)))
#define lsapi_wur __attribute__((warn_unused_result))
#define lsapi_const __attribute__((const))
#define lsapi_pure __attribute__((pure))
#define lsapi_get lsapi_nn1 lsapi_wur
#define lsapi_print lsapi_nn14

#define lssizeof(tname, fname) (offsetof(tname, fname) + sizeof(((tname*)0)->fname))

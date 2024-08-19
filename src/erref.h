#pragma once

typedef struct lserref lserref_t;
typedef struct lserrbind lserrbind_t;
typedef struct lserrlambda lserrlambda_t;

typedef enum {
  LSERRTYPE_VOID = 0,
  LSERRTYPE_BINDING,
  LSERRTYPE_LAMBDA,
} lserrtype_t;

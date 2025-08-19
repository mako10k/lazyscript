#pragma once

// Minimal interface for emitting textual LLVM IR from Core IR (MVP)

#include "coreir/coreir.h"

#ifdef __cplusplus
extern "C" {
#endif

// Emit textual LLVM IR for a given Core IR program (subset). Returns 0 on success.
int lsllvm_emit_text(FILE *out, const lscir_prog_t *cir);

#ifdef __cplusplus
}
#endif

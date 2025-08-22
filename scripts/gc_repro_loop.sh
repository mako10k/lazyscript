#!/usr/bin/env bash
# Loop a target test under GC allocator to increase repro probability.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/src/lazyscript"
TEST_REL="${1:-test/t66_gc_repro_ns_access.ls}"
TEST="$ROOT/${TEST_REL}"
if [[ ! -x "$BIN" ]]; then echo "E: binary not found: $BIN" >&2; exit 1; fi
if [[ ! -f "$TEST" ]]; then echo "E: test not found: $TEST_REL" >&2; exit 1; fi
iters=${2:-100}
echo "Repro loop: $TEST_REL iters=$iters (GC allocator)" >&2
export LAZYSCRIPT_USE_LIBC_ALLOC=0
succ=0; fail=0
for i in $(seq 1 "$iters"); do
  out="$($BIN "$TEST" 2>&1 || true)"
  if grep -qE "(SIGSEGV|Segmentation fault)" <<<"$out"; then
    echo "[#${i}] CRASH"; ((fail++))
    echo "--- got"; printf "%s\n" "$out"; echo "---"
    break
  else
    ((succ++))
    if (( i % 10 == 0 )); then echo "[#${i}] ok (cumulative ok=$succ)"; fi
  fi
  # small delay to vary timings
  usleep 10000 2>/dev/null || sleep 0.01
done
exit 0

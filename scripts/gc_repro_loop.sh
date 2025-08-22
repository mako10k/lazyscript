#!/usr/bin/env bash
# Loop a target test under GC allocator to increase repro probability.
# Be careful with `set -e`: arithmetic like ((succ++)) returns 1 when result is 0.
set -uo pipefail
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
    # Avoid set -e interaction: do not rely on arithmetic return code
    succ=$((succ+1))
    if (( i % 10 == 0 )); then echo "[#${i}] ok (cumulative ok=$succ)"; fi
  fi
  # small delay to vary timings (portable)
  sleep 0.01
done
exit 0

#!/usr/bin/env bash
# Robust test runner: do not use `set -e` to avoid unexpected early exits during conditionals.
# We handle exit codes manually and aggregate results.
set -uo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
BIN="$ROOT/src/lazyscript"
FMT_BIN="$ROOT/src/lazyscript_format"

if [[ ! -x "$BIN" ]]; then
  echo "E: lazyscript binary not found: $BIN" >&2
  exit 1
fi

# Ensure runtime module search path defaults to test dir and repo root (so require "lib/..." works)
# The search order allows tests to override by pre-setting LAZYSCRIPT_PATH.
export LAZYSCRIPT_PATH="${LAZYSCRIPT_PATH:-$DIR:$ROOT}"

pass=0; fail=0

# Discover interpreter tests automatically: any test/*.ls that has a matching .out
shopt -s nullglob
cases=()
for f in "$DIR"/*.ls; do
  base="${f%.ls}"
  if [[ -f "$base.out" ]]; then
    cases+=("$(basename "$base")")
  fi
done
shopt -u nullglob

# Load skip list if present
skip=()
if [[ -f "$DIR/skip.list" ]]; then
  while IFS= read -r s; do
    [[ -z "$s" || "$s" =~ ^# ]] && continue
    skip+=("$s")
  done < "$DIR/skip.list"
fi

for name in "${cases[@]}"; do
  [[ -z "$name" ]] && continue
  src="$DIR/$name.ls"
  exp="$DIR/$name.out"
  # Skip known-bad tests
  for s in "${skip[@]}"; do
    if [[ "$name" == "$s" ]]; then
      echo "skip - $name"
      continue 2
    fi
  done
  # Optional per-test env file (key=value per line)
  if [[ -f "$base.env" ]]; then
    # shellcheck source=/dev/null
    source "$base.env"
  fi
  # If a .trace.out exists, enable eager stack printing to stabilize output
  if [[ -f "$base.trace.out" ]]; then
    export LAZYSCRIPT_TRACE_EAGER_PRINT=1
    export LAZYSCRIPT_TRACE_STACK_DEPTH=${LAZYSCRIPT_TRACE_STACK_DEPTH:-1}
  fi
  # Run program and capture all output (stdout+stderr)
  out="$("$BIN" "$src" 2>&1)"
  if diff -u <(printf "%s\n" "$out") "$exp" >/dev/null; then
    echo "ok - $name"
    ((pass++))
  else
    echo "not ok - $name"
    echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; cat "$exp"; echo "---";
    ((fail++))
  fi
done

# Discover eval (-e) tests: any test/*.ls that has a matching .eval.out
shopt -s nullglob
eval_cases=()
for f in "$DIR"/*.ls; do
  base="${f%.ls}"
  if [[ -f "$base.eval.out" ]]; then
    eval_cases+=("$(basename "$base")")
  fi
done
shopt -u nullglob

for name in "${eval_cases[@]}"; do
  [[ -z "$name" ]] && continue
  src="$DIR/$name.ls"
  exp="$DIR/$name.eval.out"
  if [[ -f "$base.env" ]]; then source "$base.env"; fi
  if [[ -f "$base.trace.out" ]]; then
    export LAZYSCRIPT_TRACE_EAGER_PRINT=1
    export LAZYSCRIPT_TRACE_STACK_DEPTH=${LAZYSCRIPT_TRACE_STACK_DEPTH:-1}
  fi
  out="$("$BIN" -e "$(cat "$src")" 2>&1)"
  if diff -u <(printf "%s\n" "$out") "$exp" >/dev/null; then
    echo "ok - eval $name"
    ((pass++))
  else
    echo "not ok - eval $name"
    echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; cat "$exp"; echo "---";
    ((fail++))
  fi
done

# Discover Core IR dump tests: any test/*.ls that has a matching .coreir.out
shopt -s nullglob
cir_cases=()
for f in "$DIR"/*.ls; do
  base="${f%.ls}"
  if [[ -f "$base.coreir.out" ]]; then
    cir_cases+=("$(basename "$base")")
  fi
done
shopt -u nullglob

for name in "${cir_cases[@]}"; do
  [[ -z "$name" ]] && continue
  src="$DIR/$name.ls"
  exp="$DIR/$name.coreir.out"
  # Run program with Core IR dump and capture all output
  out="$("$BIN" --dump-coreir "$src" 2>&1)"
  if diff -u <(printf "%s\n" "$out") "$exp" >/dev/null; then
    echo "ok - coreir $name"
    ((pass++))
  else
    echo "not ok - coreir $name"
    echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; cat "$exp"; echo "---";
    ((fail++))
  fi
done

# Optional: formatter smoke test if the binary exists and an expectation file is present
if [[ -x "$FMT_BIN" ]]; then
  fmt_src="$DIR/t04_lambda_id.ls"
  fmt_exp="$DIR/t04_lambda_id.fmt.out"
  if [[ -f "$fmt_src" && -f "$fmt_exp" ]]; then
    fmt_out="$("$FMT_BIN" "$fmt_src" 2>&1)"
    if diff -u <(printf "%s\n" "$fmt_out") "$fmt_exp" >/dev/null; then
      echo "ok - format t04_lambda_id"
      ((pass++))
    else
      echo "not ok - format t04_lambda_id"
      echo "--- got"; printf "%s\n" "$fmt_out"; echo "--- exp"; cat "$fmt_exp"; echo "---";
      ((fail++))
    fi
  fi
fi

# Optional: Core IR evaluator smoke tests (if available)
if "$BIN" --help 2>&1 | grep -q -- "--eval-coreir"; then
  # Reuse existing tests that are evaluator-friendly
  out="$("$BIN" --eval-coreir "$DIR/t05_let_without_keyword.ls" 2>&1)"
  if [[ "$out" == "3"* ]]; then
    echo "ok - coreir-eval t05_let_without_keyword"
    ((pass++))
  else
    echo "not ok - coreir-eval t05_let_without_keyword"
    echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; echo "3"; echo "---";
    ((fail++))
  fi

  # Token-gated effect via chain: should print and return unit under evaluator
  out="$("$BIN" --eval-coreir "$DIR/t08_coreir_chain.ls" 2>&1)"
  if diff -u <(printf "%s\n" "$out") "$DIR/t08_coreir_chain.out" >/dev/null; then
    echo "ok - coreir-eval t08_coreir_chain"
    ((pass++))
  else
    echo "not ok - coreir-eval t08_coreir_chain"
    echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; cat "$DIR/t08_coreir_chain.out"; echo "---";
    ((fail++))
  fi

  # return should just pass through the value
  out="$("$BIN" --eval-coreir "$DIR/t09_coreir_return.ls" 2>&1)"
  if diff -u <(printf "%s\n" "$out") "$DIR/t09_coreir_return.out" >/dev/null; then
    echo "ok - coreir-eval t09_coreir_return"
    ((pass++))
  else
    echo "not ok - coreir-eval t09_coreir_return"
    echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; cat "$DIR/t09_coreir_return.out"; echo "---";
    ((fail++))
  fi
fi

# Optional: Core IR typechecker tests (if available)
if "$BIN" --help 2>&1 | grep -q -- "--typecheck"; then
  # Discover any test/*.ls that has a matching .type.out
  shopt -s nullglob
  type_cases=()
  for f in "$DIR"/*.ls; do
    base="${f%.ls}"
    if [[ -f "$base.type.out" ]]; then
      type_cases+=("$(basename "$base")")
    fi
  done
  shopt -u nullglob

  for name in "${type_cases[@]}"; do
    [[ -z "$name" ]] && continue
    src="$DIR/$name.ls"
    exp="$DIR/$name.type.out"
    out="$("$BIN" --typecheck "$src" 2>&1)"
    if diff -u <(printf "%s\n" "$out") "$exp" >/dev/null; then
      echo "ok - typecheck $name"
      ((pass++))
    else
      echo "not ok - typecheck $name"
      echo "--- got"; printf "%s\n" "$out"; echo "--- exp"; cat "$exp"; echo "---";
      ((fail++))
    fi
  done
fi

if [[ $fail -eq 0 ]]; then
  exit 0
else
  exit 1
fi

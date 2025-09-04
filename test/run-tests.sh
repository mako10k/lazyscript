#!/usr/bin/env bash
# Robust test runner with filters, timing, and optional JUnit export.
# Note: Do not use `set -e` to avoid early exits during conditionals; we aggregate results.
set -uo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
BIN="$ROOT/src/lsi"
FMT_BIN="$ROOT/src/lsi-fmt"

if [[ ! -x "$BIN" ]]; then
  echo "E: lsi binary not found: $BIN" >&2
  exit 1
fi

# --- CLI options -------------------------------------------------------------
# -k/--keep REGEX     : include only tests whose name matches REGEX
# -x/--exclude REGEX  : exclude tests whose name matches REGEX
# --list              : list matched tests and exit
# --shuffle [SEED]    : randomize test order (uses shuf); optional numeric seed
# --junit FILE        : write JUnit XML to FILE
# -v/--verbose        : verbose logs (echo command, durations)

KEEP_RE=""
EXCL_RE=""
DO_LIST=0
DO_SHUFFLE=0
SHUF_SEED=""
JUNIT_FILE=""
VERBOSE=0

while (( $# )); do
  case "$1" in
    -k|--keep)
      KEEP_RE="$2"; shift 2;;
    -x|--exclude)
      EXCL_RE="$2"; shift 2;;
    --list)
      DO_LIST=1; shift;;
    --shuffle)
      DO_SHUFFLE=1; SHUF_SEED="${2:-}"; if [[ -n "$SHUF_SEED" && ! "$SHUF_SEED" =~ ^[0-9]+$ ]]; then SHUF_SEED=""; fi; if [[ -n "$SHUF_SEED" ]]; then shift 2; else shift; fi;;
    --junit)
      JUNIT_FILE="$2"; shift 2;;
    -v|--verbose)
      VERBOSE=1; shift;;
    --)
      shift; break;;
    *)
      # ignore unknown to preserve backward compat
      shift;;
  esac
done

# Ensure runtime module search path defaults to test dir and repo root (so require "lib/..." works)
# The search order allows tests to override by pre-setting LAZYSCRIPT_PATH.
export LAZYSCRIPT_PATH="${LAZYSCRIPT_PATH:-$DIR:$ROOT}"

# No default init script. Tests must rely on runtime/plugin-only prelude.
export LAZYSCRIPT_INIT="${LAZYSCRIPT_INIT:-}"

# Default allocator: prefer libc for stable tests unless the caller/CI explicitly sets it.
# Note: This is a test harness setting only. It MUST NOT drive changes to product defaults.
# CI matrix can set LAZYSCRIPT_USE_LIBC_ALLOC=0 to enable GC jobs.
export LAZYSCRIPT_USE_LIBC_ALLOC="${LAZYSCRIPT_USE_LIBC_ALLOC:-1}"

# Per-test timeout (seconds). Default 10 if not provided.
TEST_TIMEOUT="${LAZYSCRIPT_TEST_TIMEOUT:-10}"

# Helper: run a command with timeout if available; capture stdout+stderr
run_with_timeout_capture() {
  if command -v timeout >/dev/null 2>&1; then
    timeout -k 1 "${TEST_TIMEOUT}s" "$@" 2>&1
  else
    "$@" 2>&1
  fi
}

pass=0; fail=0

# Logging: write last run to a stable file for tooling
LAST_LOG="$DIR/run-tests.last.log"
: >"$LAST_LOG"  # truncate

logline() {
  # tee to stdout and last log
  echo -e "$*" | tee -a "$LAST_LOG"
}

# Normalize absolute paths in outputs so CI and local paths compare stably
normalize_stream() {
  # Prefer GITHUB_WORKSPACE when available (e.g., CI: /home/runner/work/<repo>/<repo>)
  local gw="${GITHUB_WORKSPACE:-}"
  if [[ -n "$gw" ]]; then
    sed -E \
      -e "s|$ROOT|<ROOT>|g" \
      -e "s|$gw|<ROOT>|g" \
  -e 's|/workspaces/lazyscript|<ROOT>|g' \
      -e 's|/home/runner/work/[^/]+/[^/]+|<ROOT>|g'
  else
    sed -E \
      -e "s|$ROOT|<ROOT>|g" \
  -e 's|/workspaces/lazyscript|<ROOT>|g' \
      -e 's|/home/runner/work/[^/]+/[^/]+|<ROOT>|g'
  fi
}

# Discover interpreter tests automatically: any test/**/*.ls that has a matching .out
# Backward compatible with flat layout. Stable sort for deterministic order.
mapfile -t case_files < <(find "$DIR" -type f -name '*.ls' -printf '%P\n' | sort)
cases=()
for rel in "${case_files[@]}"; do
  base_noext="${rel%.ls}"
  if [[ -f "$DIR/$base_noext.out" ]]; then
    cases+=("$base_noext")
  fi
done

# Apply include/exclude filters
if [[ -n "$KEEP_RE" ]]; then
  mapfile -t cases < <(printf '%s\n' "${cases[@]}" | grep -E "$KEEP_RE" || true)
fi
if [[ -n "$EXCL_RE" ]]; then
  mapfile -t cases < <(printf '%s\n' "${cases[@]}" | grep -Ev "$EXCL_RE" || true)
fi

# Shuffle if requested
if (( DO_SHUFFLE )); then
  if command -v shuf >/dev/null 2>&1; then
    if [[ -n "$SHUF_SEED" ]]; then
      mapfile -t cases < <(printf '%s\n' "${cases[@]}" | shuf --random-source=<(yes "$SHUF_SEED") )
    else
      mapfile -t cases < <(printf '%s\n' "${cases[@]}" | shuf)
    fi
  fi
fi

if (( DO_LIST )); then
  printf '%s\n' "${cases[@]}"
  exit 0
fi

# Load skip list if present
skip=()
# Support root skip.list and per-directory skip.list with path-aware entries.
if [[ -f "$DIR/skip.list" ]]; then
  while IFS= read -r s; do
    [[ -z "$s" || "$s" =~ ^# ]] && continue
    skip+=("$s")
  done < "$DIR/skip.list"
fi
while IFS= read -r sl; do
  [[ -z "$sl" ]] && continue
  d="$(dirname "$sl")"
  if [[ -f "$DIR/$d/skip.list" ]]; then
    while IFS= read -r s; do
      [[ -z "$s" || "$s" =~ ^# ]] && continue
      # Prefix with directory for disambiguation
      skip+=("$d/${s}")
    done < "$DIR/$d/skip.list"
  fi
done < <(printf '%s\n' "${cases[@]}")

for name in "${cases[@]}"; do
  [[ -z "$name" ]] && continue
  src="$DIR/$name.ls"
  exp="$DIR/$name.out"
  base="$DIR/$name"
  # Skip known-bad tests
  for s in "${skip[@]}"; do
    if [[ "$name" == "$s" || "$(basename "$name")" == "$s" ]]; then
    logline "skip - $name"
      continue 2
    fi
  done
  start_ms=$(date +%s%3N)
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
  # Optional per-test CLI args (from .env): set LAZYSCRIPT_ARGS="-s ..."
  add_args=()
  if [[ -n "${LAZYSCRIPT_ARGS:-}" ]]; then
    # shellcheck disable=SC2206
    add_args=($LAZYSCRIPT_ARGS)
  fi
  out="$(run_with_timeout_capture "$BIN" "${add_args[@]}" "$src")"
  if diff -u <(printf "%s\n" "$out" | normalize_stream) <(normalize_stream < "$exp") >/dev/null; then
  dur_ms=$(( $(date +%s%3N) - start_ms ))
  logline "ok - $name${VERBOSE:+ (${dur_ms}ms)}"
    ((pass++))
  else
  dur_ms=$(( $(date +%s%3N) - start_ms ))
  logline "not ok - $name${VERBOSE:+ (${dur_ms}ms)}"
  logline "--- got"; printf "%s\n" "$out" | normalize_stream | tee -a "$LAST_LOG" >/dev/null; logline "--- exp"; normalize_stream < "$exp" | tee -a "$LAST_LOG" >/dev/null; logline "---"
    ((fail++))
  fi
done

# Discover eval (-e) tests: any test/**/*.ls that has a matching .eval.out
eval_cases=()
for rel in "${case_files[@]}"; do
  base_noext="${rel%.ls}"
  if [[ -f "$DIR/$base_noext.eval.out" ]]; then
    eval_cases+=("$base_noext")
  fi
done

for name in "${eval_cases[@]}"; do
  [[ -z "$name" ]] && continue
  # Apply filters on eval cases too
  if [[ -n "$KEEP_RE" ]]; then [[ "$name" =~ $KEEP_RE ]] || continue; fi
  if [[ -n "$EXCL_RE" ]]; then [[ "$name" =~ $EXCL_RE ]] && continue; fi
  src="$DIR/$name.ls"
  exp="$DIR/$name.eval.out"
  base="$DIR/$name"
  if [[ -f "$base.env" ]]; then source "$base.env"; fi
  if [[ -f "$base.trace.out" ]]; then
    export LAZYSCRIPT_TRACE_EAGER_PRINT=1
    export LAZYSCRIPT_TRACE_STACK_DEPTH=${LAZYSCRIPT_TRACE_STACK_DEPTH:-1}
  fi
  add_args=()
  if [[ -n "${LAZYSCRIPT_ARGS:-}" ]]; then
    # shellcheck disable=SC2206
    add_args=($LAZYSCRIPT_ARGS)
  fi
  out="$(run_with_timeout_capture "$BIN" "${add_args[@]}" -e "$(cat "$src")")"
  if diff -u <(printf "%s\n" "$out" | normalize_stream) <(normalize_stream < "$exp") >/dev/null; then
  logline "ok - eval $name"
    ((pass++))
  else
  logline "not ok - eval $name"
  logline "--- got"; printf "%s\n" "$out" | normalize_stream | tee -a "$LAST_LOG" >/dev/null; logline "--- exp"; normalize_stream < "$exp" | tee -a "$LAST_LOG" >/dev/null; logline "---"
    ((fail++))
  fi
done


# Optional: formatter smoke test if the binary exists and an expectation file is present
if [[ -x "$FMT_BIN" ]]; then
  fmt_src="$DIR/t04_lambda_id.ls"
  fmt_exp="$DIR/t04_lambda_id.fmt.out"
  if [[ -f "$fmt_src" && -f "$fmt_exp" ]]; then
    fmt_out="$("$FMT_BIN" "$fmt_src" 2>&1)"
    if diff -u <(printf "%s\n" "$fmt_out" | normalize_stream) <(normalize_stream < "$fmt_exp") >/dev/null; then
      echo "ok - format t04_lambda_id"
      ((pass++))
    else
      echo "not ok - format t04_lambda_id"
      echo "--- got"; printf "%s\n" "$fmt_out" | normalize_stream; echo "--- exp"; normalize_stream < "$fmt_exp"; echo "---";
      ((fail++))
    fi
  fi
fi




# Optional JUnit export
if [[ -n "$JUNIT_FILE" ]]; then
  total=$((pass+fail))
  {
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    echo "<testsuite name=\"lazyscript\" tests=\"$total\" failures=\"$fail\">"
    # Derive cases from last log lines
    # We only encode basic pass/fail without durations for simplicity here
    grep -E '^(ok|not ok) - ' "$LAST_LOG" | while read -r line; do
      status=${line%% -*}; rest=${line#*- };
      name="$rest"
      if [[ "$status" == "ok" ]]; then
        echo "  <testcase name=\"$name\"/>"
      else
        # Collect the following got/exp blocks for context until next status or EOF
        echo "  <testcase name=\"$name\">"
        echo "    <failure><![CDATA["
        awk -v start="not ok - $name" 'p==1{print} $0==start{p=1} /^ok - |^not ok - |^skip - /{if ($0!=start && p==1){exit}}' "$LAST_LOG"
        echo "]]></failure>"
        echo "  </testcase>"
      fi
    done
    echo "</testsuite>"
  } > "$JUNIT_FILE" || true
fi

exit $(( fail==0 ? 0 : 1 ))

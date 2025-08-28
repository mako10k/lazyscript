#!/usr/bin/env bash
set -euo pipefail
# Scan for typical implicit fallback patterns and forbidden fixed identifiers in coreir/runtime.
# This is a heuristic guard; refine over time. Allowlist test and docs.

shopt -s globstar nullglob
bad=0

scan_file() {
  local f="$1"
  # Skip generated or third-party or tests
  [[ "$f" == test/** ]] && return 0
  [[ "$f" == docs/** ]] && return 0

  # Patterns indicating name-based fallback in coreir evaluator/typechecker
  if grep -nE 'LCIR_VAL_VAR' -n "$f" | grep -q .; then
    : # presence alone isn't bad
  fi
  if grep -nE 'strcmp\([^,]+,\s*"(add|sub|concat|cons|map|head|tail|length|is_nil|println|exit)"\)' "$f" >/dev/null 2>&1; then
    echo "ERROR: Name-based builtin resolution detected in $f" >&2
    bad=1
  fi
  # Forbidden fixed identifiers in implementation logic (heuristic)
  if grep -nE '"(prelude|builtins|internal|env)"' "$f" >/dev/null 2>&1; then
    echo "WARN: Possible fixed identifier reference in $f (check and justify or remove)." >&2
  fi
}

for f in src/**/*.c src/**/*.h; do
  scan_file "$f"
done

if [[ $bad -ne 0 ]]; then
  echo "Forbidden implicit fallback detected. Please remove name-based dispatch and use explicit import/registry." >&2
  exit 1
fi

echo "OK: no implicit fallback patterns found."

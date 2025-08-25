#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
BIN="$ROOT/src/lazyscript"
FMT_BIN="$ROOT/src/lazyscript_format"

if [[ ! -x "$BIN" || ! -x "$FMT_BIN" ]]; then
  echo "E: binaries not found: $BIN or $FMT_BIN" >&2
  exit 1
fi

# Normalize paths for stable diffs
normalize() {
  sed -E -e "s|$ROOT|<ROOT>|g" -e 's|/workspaces/lazyscript|<ROOT>|g' -e 's|/home/runner/work/[^/]+/[^/]+|<ROOT>|g'
}

pass=0; fail=0

# 1) Formatter: pure dot-chain argument should drop parens
src1="$DIR/r90_fmt_pure_dot_chain.ls"
exp1="$DIR/r90_fmt_pure_dot_chain.fmt.out"
got1="$($FMT_BIN "$src1" 2>&1 | normalize)"
if diff -u <(printf "%s\n" "$got1") <(normalize < "$exp1") >/dev/null; then
  echo "ok - format r90_fmt_pure_dot_chain"
  ((pass++))
else
  echo "not ok - format r90_fmt_pure_dot_chain"; echo "--- got"; printf "%s\n" "$got1"; echo "--- exp"; normalize < "$exp1"; echo "---"
  ((fail++))
fi

# 2) Parser: disallow adjacent .sym that make literal head (~a ~b.c.d.e .f.g)
src2="$DIR/r91_parse_dotdot_invalid.ls"
got2="$($BIN "$src2" 2>&1 | normalize || true)"
if grep -Eiq "syntax error|cannot be applied|consecutive dot-symbol arguments are not allowed" <<<"$got2"; then
  echo "ok - parse r91_parse_dotdot_invalid"
  ((pass++))
else
  echo "not ok - parse r91_parse_dotdot_invalid"; echo "--- got"; printf "%s\n" "$got2"; echo "--- exp (should be a parse error)"; echo "<syntax error>"; echo "---"
  ((fail++))
fi

if [[ $fail -eq 0 ]]; then
  exit 0
else
  exit 1
fi

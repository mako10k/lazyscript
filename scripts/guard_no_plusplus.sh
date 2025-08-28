#!/usr/bin/env bash
set -euo pipefail

# Guard against accidentally reintroducing the language-level "++" operator.
#
# Scope: Only checks grammar sources (parser.y, lexer.l). It allows C/C++ "++" in generated C files.
# Strategy:
#  - Forbid quoted '++' literals in grammar sources (e.g., '++' or "++").
#  - Forbid token identifiers LSTCONCAT/LSTPLUS/PLUSPLUS in parser.y, except for the known comment note.
#  - Print offending lines with file:line for fast fixing.
#
# Exit status: non-zero on violation; zero when clean.

root_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

grammar_files=(
  "$root_dir/src/parser/parser.y"
  "$root_dir/src/parser/lexer.l"
)

violations=0

echo "[guard-no-plusplus] checking grammar sources…"

# 1) Quoted '++' literal in grammar sources
if grep -R -n -E "['\"][+]\\+['\"]" "${grammar_files[@]}" ; then
  echo "ERROR: Found quoted '++' in grammar sources (should not define the operator)." >&2
  violations=1
fi

# 2) Forbidden token identifiers in parser.y (ignore the known comment line documenting removal)
token_hits=$(grep -n -E "\\b(LSTCONCAT|LSTPLUS|PLUSPLUS)\\b" "$root_dir/src/parser/parser.y" | grep -v "removed LSTCONCAT") || true
if [[ -n "$token_hits" ]]; then
  echo "$token_hits"
  echo "ERROR: Forbidden token identifiers detected in parser.y." >&2
  violations=1
fi

if [[ "$violations" -ne 0 ]]; then
  echo "[guard-no-plusplus] FAILED: please remove the above occurrences." >&2
  exit 1
else
  echo "[guard-no-plusplus] OK: no language-level '++' found in grammar."
fi

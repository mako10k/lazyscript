#!/usr/bin/env bash
set -euo pipefail

# Guard: ban the "++" operator from language sources (spec, grammar, libraries, tests)
# - Strict ban in .ls and docs
# - Parser/lexer (.y/.l): only flag token/lexeme definitions like '"++"' or token names (e.g., LSTCONCAT)
#   and ignore incidental C code increments (i++) in semantic actions.

shopt -s globstar nullglob

paths_ls_docs=(
  "docs/**/*.md"
  "docs/**/*.bnf"
  "docs/parser-bnf.md"
  "lib/**/*.ls"
  "test/**/*.ls"
)
paths_parser=(
  "src/parser/**/*.y"
  "src/parser/**/*.l"
)

violations=()

# Strict ban in docs/.ls
for gl in "${paths_ls_docs[@]}"; do
  for f in $gl; do
    [[ -f "$f" ]] || continue
    if grep -n -- '++' "$f" >/dev/null 2>&1; then
      while IFS= read -r line; do
        violations+=("$f:$line")
      done < <(grep -n -- '++' "$f")
    fi
  done
done

# Parser/lexer: only flag explicit '++' token definitions or token names
for gl in "${paths_parser[@]}"; do
  for f in $gl; do
    [[ -f "$f" ]] || continue
    if grep -nE "LSTCONCAT|\"\+\+\"|'\+\+'" "$f" >/dev/null 2>&1; then
      while IFS= read -r line; do
        violations+=("$f:$line")
      done < <(grep -nE "LSTCONCAT|\"\+\+\"|'\+\+'" "$f")
    fi
  done
done

if (( ${#violations[@]} > 0 )); then
  echo "ERROR: '++' operator is banned from language spec/grammar/libs/tests. Found in:" >&2
  printf '  %s\n' "${violations[@]}" >&2
  exit 1
fi

echo "guard_no_plusplus: OK (no occurrences)."

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

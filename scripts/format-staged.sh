#!/usr/bin/env bash
set -euo pipefail
CLANG_FORMAT_BIN=${CLANG_FORMAT_BIN:-clang-format}
mapfile -t files < <(git diff --cached --name-only --diff-filter=ACMR | grep -E '\\.(c|h|cc|cpp|hpp)$' || true)
[[ ${#files[@]} -eq 0 ]] && exit 0
for f in "${files[@]}"; do
  [[ -f "$f" ]] || continue
  "${CLANG_FORMAT_BIN}" -style=file -i "$f"
  git add "$f"
done
echo "Formatted and re-staged ${#files[@]} file(s)."
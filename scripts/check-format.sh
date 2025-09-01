#!/usr/bin/env bash
set -euo pipefail
CLANG_FORMAT_BIN=${CLANG_FORMAT_BIN:-clang-format}
mapfile -t files < <(git ls-files '*.c' '*.h' '*.cc' '*.cpp' '*.hpp')
[[ ${#files[@]} -eq 0 ]] && { echo "Formatting OK (no files)"; exit 0; }
fail=0
for f in "${files[@]}"; do
  if ! diff -q <(cat "$f") <("${CLANG_FORMAT_BIN}" -style=file "$f") >/dev/null 2>&1; then
    echo "needs format: $f"
    fail=1
  fi
done
if [[ $fail -ne 0 ]]; then
  echo "Formatting check failed. Run clang-format -i or scripts/format-staged.sh." >&2
  exit 1
fi
echo "Formatting OK"
#!/usr/bin/env bash
set -euo pipefail

FORBIDDEN=(
  "src/builtins/prelude.c"
  "src/builtins/prelude.h"
)

bad=0
for f in "${FORBIDDEN[@]}"; do
  if [ -e "$f" ]; then
    echo "ERROR: Forbidden file present: $f" >&2
    bad=1
  fi
  if git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
    echo "ERROR: File is tracked in git: $f" >&2
    bad=1
  fi
done

if [ "$bad" -ne 0 ]; then
  echo "Legacy host prelude must not exist. Use src/plugins/prelude_plugin.c only." >&2
  exit 1
fi

echo "OK: no forbidden files present."
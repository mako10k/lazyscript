#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")"/.. && pwd)"
cd "$ROOT"

echo "[check] scanning for TODO tags and spec-needed markers..."
grep -RIn --exclude-dir=.git --exclude-dir=.libs --exclude-dir=.deps -E "\[ASSUME\]|\[TEMP\]|\[SPEC-NEEDED\]" || true

echo "[check] scanning for implicit pattern-let or case without spec..."
grep -RIn --exclude-dir=.git --exclude-dir=.libs --exclude-dir=.deps -E "pattern let|case-of" src || true

echo "[check] ensure current.md exists and is non-empty..."
test -s .github/user-instruction/current.md || { echo "E: missing .github/user-instruction/current.md"; exit 2; }

echo "[check] OK (advisory)"

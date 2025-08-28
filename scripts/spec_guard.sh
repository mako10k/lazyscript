#!/usr/bin/env bash
set -euo pipefail

# Guard: if parser/lexer changed since last commit, require docs/spec updates unless override env is set.

if [ "${ALLOW_SPEC_CHANGE_WITHOUT_DOCS:-}" = "1" ]; then
  exit 0
fi

BASE_REF=${1:-HEAD~1}

grammar_changed() {
  git --no-pager diff --name-only "$BASE_REF" -- src/parser/lexer.l src/parser/parser.y | grep -q .
}

spec_changed() {
  git --no-pager diff --name-only "$BASE_REF" -- docs/parser-bnf.md docs/spec/ | grep -q .
}

if grammar_changed && ! spec_changed; then
  echo "Spec Guard (local): Grammar changed but no spec/docs update detected." >&2
  echo "Please update docs/parser-bnf.md and/or docs/spec/* and commit those changes." >&2
  exit 1
fi

exit 0

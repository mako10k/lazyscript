#!/usr/bin/env bash
set -euo pipefail

# Check that specific lexer rules in src/parser/lexer.l appear in the correct order
# to avoid precedence regressions between overlapping tokens.

LEXER_FILE="$(dirname "$0")/../src/parser/lexer.l"
LEXER_FILE="$(realpath "$LEXER_FILE")"

if [[ ! -f "$LEXER_FILE" ]]; then
  echo "E: lexer file not found: $LEXER_FILE" >&2
  exit 2
fi

# Extract line numbers for key patterns
get_line() {
  local pattern="$1"
  # Use grep -n with fixed regex; print the first match line number only
  local line
  line=$(grep -n -E "${pattern}" "$LEXER_FILE" | head -n1 | cut -d: -f1 || true)
  if [[ -z "$line" ]]; then
    echo 0
  else
    echo "$line"
  fi
}

# Patterns (keep in sync with lexer.l rules)
DOT_QUOTED="^\\[\.\\]\\[\\'\\]\\([^\\\\\\'\\]|\\\\\\.\\)*\\[\\'\\] \\{"
DOT_IDENT="^\\[\.\\]\\[a-zA-Z_\\]\\[a-zA-Z0-9_\\]\\* \\{"
TILDE_QUOTED="^\\[~\\]\\[\\'\\]\\([^\\\\\\'\\]|\\\\\\.\\)*\\[\\'\\] \\{"
TILDE_IDENT="^\\[~\\]\\[a-zA-Z_\\]\\[a-zA-Z0-9_\\]\\* \\{"
PLAIN_QUOTED="^\\[\\'\\]\\([^\\\\\\'\\]|\\\\\\.\\)*\\[\\'\\] \\{"
BARE_IDENT="^\\[a-zA-Z_\\]\\[a-zA-Z0-9_\\]\\* \\{"

# Fetch first matching line numbers
read -r dot_quoted_ln < <(get_line "$DOT_QUOTED")
read -r dot_ident_ln < <(get_line "$DOT_IDENT")
read -r tilde_quoted_ln < <(get_line "$TILDE_QUOTED")
read -r tilde_ident_ln < <(get_line "$TILDE_IDENT")
read -r plain_quoted_ln < <(get_line "$PLAIN_QUOTED")
read -r bare_ident_ln < <(get_line "$BARE_IDENT")

fail=0

check_order() {
  local a_name="$1" a_ln="$2" b_name="$3" b_ln="$4"
  if [[ "$a_ln" -eq 0 ]]; then
    echo "E: rule not found: $a_name" >&2; fail=1; return
  fi
  if [[ "$b_ln" -eq 0 ]]; then
    echo "E: rule not found: $b_name" >&2; fail=1; return
  fi
  if [[ "$a_ln" -ge "$b_ln" ]]; then
    echo "E: rule order regression: expected $a_name (line $a_ln) to appear BEFORE $b_name (line $b_ln)" >&2
    fail=1
  else
    echo "OK: $a_name (line $a_ln) precedes $b_name (line $b_ln)"
  fi
}

echo "Checking lexer rule precedence in $LEXER_FILE" >&2

# Ensure specific precedence constraints
check_order "dot-quoted" "$dot_quoted_ln" "dot-ident" "$dot_ident_ln"
check_order "tilde-quoted" "$tilde_quoted_ln" "tilde-ident" "$tilde_ident_ln"
# Not strictly overlapping but good guard: plain-quoted before bare-ident
check_order "plain-quoted" "$plain_quoted_ln" "bare-ident" "$bare_ident_ln"

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

echo "All lexer precedence checks passed."

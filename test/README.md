# tests

This directory contains the test suite for lazyscript.

Structure (current + planned):
- Flat files tXX_*.ls with matching expectations (current)
- Subdirectories for categories (planned, backward compatible):
  - parser/
  - runtime/
  - ns/
  - io/
  - regression/

Conventions:
- For an interpreter test, create <name>.ls and <name>.out with exact expected stdout+stderr.
- For eval(-e) tests, add <name>.eval.out.
- Optional: <name>.env to inject env vars (key=value per line) for that test.
- Optional: <name>.trace.out turns on eager stack printing to stabilize traces.

Skip lists:
- test/skip.list applies globally.
- Each subdirectory may also have a skip.list; entries are file stems within that subdir. The runner prefixes directory name automatically.

Runner:
- run-tests.sh discovers tests recursively and runs them in stable order.
- Environment defaults:
  - LAZYSCRIPT_PATH="$DIR:$ROOT"
  - LAZYSCRIPT_INIT="$ROOT/lib/PreludeInit.ls"
  - LAZYSCRIPT_USE_LIBC_ALLOC=1
  - LAZYSCRIPT_TEST_TIMEOUT (seconds, default 10)

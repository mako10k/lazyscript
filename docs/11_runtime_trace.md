# Runtime tracing (experimental)

This prototype adds a sidecar sourcemap for runtime error traces without changing the Core IR.

- Sidecar format: JSON Lines (one JSON object per line)
  - {"src":"<path>","sl":1,"sc":1,"el":1,"ec":1}
  - Fields
    - src: source filename
    - sl/sc: start line/column (1-based)
    - el/ec: end line/column (1-based)
- Indexing: lines are implicitly indexed from 0 in creation order (matches runtime-assigned trace_id).

## How it works

- Every thunk gets a sequential trace_id when constructed.
- The evaluator pushes/pops trace_id to a small global stack during evaluation.
- On #err printing, up to N frames from this stack are printed as ` at file:line:col` if a sourcemap is loaded.

## CLI / environment

- --trace-map <file>: load a sourcemap JSONL at startup.
- --trace-stack-depth <n>: print up to n frames on error (default: 1). Env: LAZYSCRIPT_TRACE_STACK_DEPTH.
- --trace-dump <file>: write a sourcemap JSONL during evaluation in thunk creation order. Env: LAZYSCRIPT_TRACE_DUMP.

Notes:
- --trace-dump writes the file but does not auto-load it. Use --trace-map on a subsequent run to resolve frames, or provide a pre-generated map.
- For now, only reference thunks use precise locations. Other nodes emit `<unknown>` placeholders until parser/AST carry locations.

## JSONL example

{"src":"examples/hello.ls","sl":1,"sc":1,"el":1,"ec":12}
{"src":"examples/hello.ls","sl":1,"sc":14,"el":1,"ec":18}

## Roadmap

- Emit real locations for all expressions once loc is threaded through the AST.
- Per-thread trace stacks for future concurrency.
- Tests that validate formatted trace output with a known map.

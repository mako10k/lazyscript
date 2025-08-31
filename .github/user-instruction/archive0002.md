created: 2025-08-31T06:25:00Z

## Current Instruction (verbatim)

実行ランタイムがLSTIを利用する実装に変更する計画を立ててください。

## Metadata
- source: chat (user message)
- integrity: recorded verbatim without edits
- next archive index: 0002

## Assistant Context (most-recent first)
- Plan an implementation roadmap to switch runtime to use LSTI for fast loading, while keeping LSTB for portability.
- Define phases: image spec finalization, header/constants, encoder, minimal loader, integration flag, tests/benchmarks, rollout.

## Prior Context Summary (compact)
- LSTB spec exists and doc now includes LSTI high-level spec; runtime contains LSTB subset I/O in `thunk.c`.
- Choice operators and caret catch are stable; build system is Automake/libtool.
integrity_note: Archived from current.md per protocol.

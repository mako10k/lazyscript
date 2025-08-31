---
created: 2025-08-31T05:36:10Z
---

## Current Instruction (verbatim)

実行時Thunkバイナリーデータ案を作って。

## Metadata
- source: chat (user message)
- integrity: recorded verbatim without edits
- next archive index: 0001

## Assistant Context (most-recent first)
- Draft a runtime Thunk binary data format: header, per-kind payload layouts, flags, versioning, references/relocations, and examples.
- Ensure compatibility with current runtime thunk kinds (alge, appl, choice lambda/expr/catch, lambda, ref, literals) and WHNF caching.

## Prior Context Summary (compact)
- Language now has lambda-only '|' and expr '||' plus catch '^|' with right-assoc; runtime thunk allocation bug fixed.
- Tests for '^|' pass (t93/t94); broader harness has unrelated skips. '~prelude' naming unified.

---
created: 2025-08-29T11:35:50Z
---

## Current Instruction (verbatim)

現在エラーが出てるのはスキップ、ただし、 ~Prelude はお前、もう存在しない報告が虚偽でしたね。。
~prelude が正しいはずです。
それだけは直して。

## Metadata
- source: chat (user message)
- integrity: recorded verbatim without edits
- next archive index: 0002

## Assistant Context (most-recent first)
- Replace erroneous references to `~Prelude` with `~prelude` in tests/scripts; verify by running harness.
- Keep unrelated failing tests skipped; only fix the naming issue requested.

## Prior Context Summary (compact)
- '^|' operator stabilized; earlier changes split '|' vs '||' semantics and restricted '|' to lambdas.
- Some tests still fail due to prelude/plugin/env alignment; this change addresses name casing only.

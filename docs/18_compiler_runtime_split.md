# コンパイラ/ランタイム分離計画（Core IR）

目的: フロントエンド（コンパイル: LazyScript → Core IR）とバックエンド（ランタイム: Core IR 実行）をプロセス/バイナリとして分離し、IR の安定化とツール連携/保守性を向上する。

## ゴール像（バイナリ/ライブラリ）

- バイナリ
  - `lazyscriptc`（コンパイラ）: .ls から Core IR を生成（`--emit-coreir` 既定）、`--typecheck`、`--strict-effects` 等。
  - `lscoreir`（ランタイム）: Core IR を入力として実行。将来はテキストIR/バイナリIRの両対応。初期段階では機能限定。
- ライブラリ
  - `libls-common`（共通 / 既存）
  - `libls-coreir-frontend`（AST→Core IR 生成 / 既存 lscir_lower_* を再利用）
  - `libls-coreir-runtime`（Core IR 評価器 / 既存 lscir_eval を再利用）

## IR 取り扱い

- 仕様: `docs/04_core_ir.md` を準拠（ヘッダ行に `version` 等を追加予定）。
- 互換性: ランタイムは対応バージョン範囲を明示し、不一致は診断の上エラー。
- 入出力: UTF-8 テキスト IR を既定。将来はバイナリ（例: msgpack）も検討。

## CLI 概要

- lazyscriptc
  - 例: `lazyscriptc input.ls > out.coreir` / `lazyscriptc -e "(…)" --typecheck`
  - オプション: `-e/--eval`, `--emit-coreir`(既定), `--typecheck`, `--strict-effects`, `--debug`, `--help` など。
- lscoreir
  - 例: `lazyscriptc input.ls | lscoreir`
  - 初期段階: テキスト IR リーダは TBD。暫定で `--from-ls` により .ls を受け取り IR 経由で評価（動作確認・移行用）。

## フェーズ計画

1. 設計/ドキュメント整備（本ドキュメント）
2. ビルド分離の土台: 新バイナリ `lazyscriptc`/`lscoreir` の追加（最小機能）
3. ランタイムの IR テキスト読取の実装（パーサ or 読み込みAPI）
4. テスト/CI を 2段（compile→run）モード追加。主要ケースは直実行と IR 経由の一致を確認
5. VS Code 拡張の内部呼び出しを 2段に切替
6. 旧 `--eval-coreir` の段階的デプリケーション

## リスクと緩和

- IR 仕様の揺れ: バージョニング＋互換テストを CI に導入
- require/import の責務: include は前段、require/import は後段で解決の原則を明文化
- パフォーマンス: パイプ既定、巨大 IR はファイル受け渡しを推奨

## 現段階の実装方針（フェーズ2）

- `lazyscriptc`: 既存のフロントエンド/IR生成/型検査を呼び出して Core IR を出力。
- `lscoreir`: 将来の IR テキスト読取を見据えた骨格を実装。暫定で `--from-ls` 入力のみ評価可能。

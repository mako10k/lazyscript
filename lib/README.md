# Standard Library (lib/) – 指針と現状

目的
- 互換性を保ちながら、純粋なモジュール（namespace value）中心へ整理。
- 効果のある定義（環境更新）はプレリュード経由・別ファイルに隔離。
- 命名と読み込みパターンを統一し、依存側からの参照を安定化。

基本原則
- すべての「通常モジュール」は純粋なリテラル `{ .name = expr; ... }` を返す。
  - 中で参照だけ行うのは可（例: `(~prelude nsMembers)` への参照）。
  - 環境更新（nsnew/nsdef/def など副作用）は行わない。
- 依存は `requirePure "lib/…"` を基本とし、効果のある `require` はデモ/互換用途に限る。
- 公開 API は記号キー `.name` のみ（シンボルキー限定）。

読み込みの推奨形
- 直接: `(~prelude requirePure) "lib/option.ls"` など。
- 集約（新設）: `(~prelude requirePure) "lib/std.ls"` 経由で主要モジュールを取得。

現状モジュール一覧（2025-08）
- List.ls（純）: リスト操作（foldl/foldr/map/filter/append/reverse/flatMap）。
- option.ls（純）: `Option` コンストラクタと map/flatMap/getOrElse。
- result.ls（純）: `Result` コンストラクタと map/flatMap/withDefault。
- ns.ls（純）: 名前空間ユーティリティ（nsMembers/nsHas/nsGetOr）。
- string.ls（純/最小）: `.length` のみ（将来拡張予定）。
- list.ls（互換/副作用）: グローバル `cons/nil` を定義するレガシー互換ファイル（非推奨）。
- ns_local_demo.ls（サンプル）: nslit の暗黙ローカル活用例。

非推奨ポリシー（段階移行）
- `lib/list.ls` は互換のため当面維持しつつ、`lib/List.ls` の使用を推奨。
- 副作用を伴うユーティリティは別ファイルに隔離し、純モジュール側からは参照のみ。

今後の整理方針（概要）
- 命名の一貫化: ADT/データ型モジュールは PascalCase（例: List）、ユーティリティは lower-case を暫定維持し、将来の大きな変更時に統一。
- モジュール目録の拡充: `std.ls` に安定提供面を集約し、追加モジュールはまず `std.ls` に入口を用意。
- ドキュメント整備: 各モジュール冒頭にエクスポート一覧と純/効果の注意書きを追記。

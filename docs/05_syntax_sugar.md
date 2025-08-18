# シンタックスシュガー: ~~sym → (~prelude sym)

- 目的: `(~prelude println)` や `(~prelude return)` などの頻出呼び出しを短縮する。
- 書式: `~~<識別子>`（例: `~~println`, `~~exit`, `~~chain`, `~~return`）
- 展開: `~~sym` は `(~prelude sym)` に等価。
- 制限: `~~` の直後は識別子（`[a-zA-Z_][a-zA-Z0-9_]*`）のみ。文字列・記号・ドット区切りは不可。

## 実装メモ
- lexer (`src/parser/lexer.l`) に `LSTPRELUDESYM` トークンを追加。
- parser (`src/parser/parser.y`) の `efact` に `LSTPRELUDESYM` 規則を追加し、`(~prelude sym)` の適用にデスガー。
- Bison 警告の `%expect` を 36 に更新（新規トークン導入で競合カウントが増えたため）。

## 例
- `(~~println "hi");` → `hi` と `()` を出力。
- `(~~return 7);` → `7`。

## 名前空間の切り替え（将来拡張）
- `~~` が展開する先の名前空間（既定: `prelude`）は、ランタイム引数または環境変数で切替可能にする方針。
  - 例: `--sugar-namespace somens` または `LAZYSCRIPT_SUGAR_NS=somens`
- 実装方法:
  1. `lsscan_t` に `const char *ls_sugar_ns` を追加し、既定値を "prelude" に設定。
  2. CLI から `--sugar-namespace` でセット。`lsparse_*` 呼び出し時に `lsscan` へ渡す。
  3. `parser.y` の `LSTPRELUDESYM` 展開で `lsref_new(ls_sugar_ns, loc)` を参照。
- 互換性: 未指定なら従来通り `prelude` を用いるため後方互換。

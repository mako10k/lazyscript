# シンタックスシュガー: ~~sym → (~prelude sym)


## 実装メモ
- Bison 警告の `%expect` を 36 に更新（新規トークン導入で競合カウントが増えたため）。
---

# do ブロック（予約語なしの do 記法）

- 目的: 予約語を導入せずに Haskell 風の do 記法を提供する。
- 書式: `{ stmt1 ; stmt2 ; ... ; stmtN }`
  - ステートメント `stmt` は以下のいずれか
    - 式: `E`
    - 束縛: `x <- E`（左辺は識別子。将来拡張でパターンも検討可能）
- 末尾の意味: ブロック末尾は自動で `(~<ns> return)` に包む（暗黙の return）。
- デシュガー（<ns> は糖衣名前空間: 既定 prelude）
  - `E ; rest`   ⇒ `(~<ns> chain) E (\ _ -> rest')`
  - `x <- E ; rest` ⇒ `(~<ns> bind) E (\ x -> rest')`
  - `tail E`    ⇒ `(~<ns> return) E`
- 空ブロック `{}` は現在エラー（将来 `return ()` へ拡張可）。

## 例

```
{ ~~println "A"; ~~println "B" };
```

出力:

```
A
B
()
```

```
{ x <- ~add 1 2; ~~println ~x };
```

出力:

```
3
()
```

## 実装メモ
- lexer (`src/parser/lexer.l`): `"<-"` を `LSTLEFTARROW` として追加。
- parser (`src/parser/parser.y`): `{ dostmts }` を `efact` の一分岐に追加し、
  `dostmts` を右結合で `(~ns chain|bind|return)` に構築してデシュガー。
- 糖衣名前空間は `lsscan_get_sugar_ns(...)` を使用（`--sugar-namespace` / `LAZYSCRIPT_SUGAR_NS`）。
- ランタイム（インタプリタ）に `prelude.bind/2` を追加。
- Core IR evaluator 側にも `bind/2` を追加（トークン有効化とラムダ適用）。

## 注意
- 末尾セミコロンはエラー。必要なら許容し `return ()` に展開する改善余地あり。
- 左辺は現状識別子のみ。`(~)` 参照（`pref`）も許容しているが、一般化は段階的に検討。


## 例
- `(~~println "hi");` → `hi` と `()` を出力。
- `(~~return 7);` → `7`。

## 名前空間の切り替え（実装済み）
- `~~` の展開先名前空間（既定: `prelude`）は、CLI もしくは環境変数で切替可能です。
  - 例: `--sugar-namespace somens` または `LAZYSCRIPT_SUGAR_NS=somens`
- 実装メモ:
  1. `lsscan_t` に `ls_sugar_ns` を追加し既定値を "prelude" に設定。
  2. CLI `--sugar-namespace` / 環境変数 `LAZYSCRIPT_SUGAR_NS` で上書き。
  3. `parser.y` の `LSTPRELUDESYM` 展開で `ls_sugar_ns` を参照。
  

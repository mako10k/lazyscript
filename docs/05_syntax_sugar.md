# シンタックスシュガー概要（~~ と !）

LazyScript には次の 2 系統の糖衣があり、どちらも「現在の糖衣名前空間」に展開されます（既定は `prelude`。CLI `--sugar-namespace` または環境変数 `LAZYSCRIPT_SUGAR_NS` で切替）。

- 純粋 API 参照: `~~sym` → `(~<ns> sym)`
  - 例: `((!println) "hi")` は `(~prelude println) "hi"` に展開。
- 環境操作 API 参照: `!sym` → `(~<ns> .env .sym)`
  - 例: `!require "lib/Foo.ls"` は `(~prelude .env .require) "lib/Foo.ls"` に展開。

いずれも「関数適用は通常の括弧」で行います。`~~sym` 自体は値（関数）です。

注意: シェルの履歴展開の都合で、CLI `-e` に `!{ ... }` や `!require` を直接書く場合はクォートしてください。

関連: 詳細な背景とエクスポート一覧は `docs/12_builtins_and_prelude.md` を参照してください。

## do ブロック（`!{ ... }`）

予約語なしの do 記法を `!{ ... }` で提供します。ブロック内部は次にデシュガーされ、順序効果を明示します（`<ns>` は糖衣名前空間）。

- 文の形:
  - 式: `E`
  - 束縛: `x <- E`（左辺は識別子）
- デシュガー規則（右結合）:
  - `E ; rest`        ⇒ `(~<ns> chain) E (\ _ -> rest')`
  - `x <- E ; rest`   ⇒ `(~<ns> bind)  E (\ x -> rest')`
  - 末尾 `E`          ⇒ `(~<ns> return) E`

例:

```
!{ !println "A"; !println "B" };
```

出力:

```
A
B
()
```

環境 API（`!sym`）と併用する例:

```
!{ !require "test/lib_req.ls"; !println "ok" };
```

出力:

```
OK-from-lib
ok
()
```

## 実装の要点（参考）

- `~~sym` は lexer の `LSTPRELUDE_*` 系トークンで捕捉し、`(~<ns> sym)` に構文展開。
- `!sym` は `LSTENVOP` として字句化し、`(~<ns> .env .sym)` に構文展開。
- `!{ ... }` は `bind/chain/return` を用いた式列にデシュガー（`prelude` がそれらを再エクスポート）。

## 糖衣名前空間の切り替え

- 既定は `prelude`。
- 切替: `--sugar-namespace <ns>` または `LAZYSCRIPT_SUGAR_NS=<ns>`。

例（CLI で一時切替）:

```
src/lazyscript --sugar-namespace prelude -e '!{ !println "hi" }'
```

## 環境 API の小例（!def）

```
!{
  !def .x 42;
  !println (~to_str ~x)
};
```

出力:

```
42
()
```
  

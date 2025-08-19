# Namespaces

LazyScript には軽量な名前空間があり、値を格納する辞書として使えます。プリリュード経由で作成・定義・参照します。

- 作成: `(~prelude nsnew)` は新しい空の名前空間値を返します。
- 定義: `(~prelude nsdef)` は指定 NS にシンボル名で値を定義します。
- 参照: `(~NS sym)` で NS の中からシンボルを値として取り出せます（関数でも可）。
- シュガー: `~~sym` は `(~prelude sym)` の短縮。`LAZYSCRIPT_SUGAR_NS` で名前空間を切替可能（デフォルト prelude）。

## 例

```
!{
  let ns = (~prelude nsnew);
  ((~prelude nsdef) ns 'x 42);
  ~~println ((~ns 'x));      # => 42
};
```

関数の格納と呼び出し:

```
!{
  let ns = (~prelude nsnew);
  ((~prelude nsdef) ns 'inc (\n -> n + 1));
  ~~println (((~ns 'inc) 41));  # => 42
};
```

## 注意
- `nsdef` は効果（環境の更新）を伴います。`strict-effects` 有効時は `seq/chain` でトークンを通してください。
- `(~NS sym)` の `sym` は素のシンボル（`'name`）を期待します。
- 名前空間レジストリは実装内で管理され、`ns` 値を経由してアクセスします。

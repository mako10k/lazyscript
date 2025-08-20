# Namespaces

LazyScript には軽量な名前空間があり、値を格納する辞書として使えます。プリリュード経由で作成・定義・参照します。

- 参照: `(~NS sym)` で NS の中からシンボルを値として取り出せます（関数でも可）。
- シュガー: `~~sym` は `(~prelude sym)` の短縮。`LAZYSCRIPT_SUGAR_NS` で名前空間を切替可能（デフォルト prelude）。

## 名前付き名前空間（既存）

- 作成: `~~nsnew NS` は `NS` という名前の名前空間を作り、`(~NS sym)` で参照可能にします。
- 定義: `~~nsdef NS Foo 42` で `NS` に `Foo` を定義。

例:

```
!{ ~~nsnew NS; ~~nsdef NS Foo 42; ~~println (~to_str ((~NS Foo))) };
```

## 無名（匿名）名前空間（新機能）

- 作成: `~~nsnew0` は新しい「名前空間値」を返します。これは 1 引数のビルトインで、`(~NS sym)` と同様に動作します。
- 定義: `~~nsdefv NS Foo 42` で、`NS` が「名前空間値」の場合に直接定義できます。第1引数に名前（シンボル）を渡すと、従来のレジストリから名前付き NS も参照可。
- セッター: `(~NS __set)` は `(sym val) -> ()` なセッタービルトインを返します。

例:

```
!{
  ns <- (~~nsnew0);
  (~~nsdefv ns Foo 42);
  ~~println (~to_str ((~ns Foo)))  # => 42
};
```

セッター経由:

```
!{
  ns <- (~~nsnew0);
  ((~ns __set) Foo 42);
  ~~println (~to_str ((~ns Foo)))  # => 42
};
```

## 注意
- `nsdef`/`nsdefv` は効果（環境の更新）を伴います。`strict-effects` 有効時は `seq/chain` でトークンを通してください。
- `(~NS sym)` の `sym` は素のシンボル（`'name`）を期待します。
- 無名 NS は値として渡せるため、クロージャや局所スコープでの辞書用途に向きます。

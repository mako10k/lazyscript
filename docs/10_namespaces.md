# Namespaces（設計更新案）

LazyScript の名前空間は「定数キー const から値への写像」を提供します。名前空間には2種類あります。

- Named namespace（レジストリ登録型）: 名前でグローバル登録された名前空間。
- Namespace value（無名/第一級値）: 値として渡せる名前空間。

本書では両者の共通仕様と相違点、作成・定義・参照・列挙の各操作を示します。

- 参照（共通）: `(~NS const)` で NS から値を取得（関数でも可）。
- シュガー: `~~sym` は `(~prelude sym)` の短縮。`LAZYSCRIPT_SUGAR_NS` で切替可能（デフォルト prelude）。

キー `const` は次のいずれかの定数型です: `int | str | constr(0-arity) | symbol`。シンボルは `.name` のリテラルで表し、インターンされ `==` 比較できます。

## 名前付き名前空間（Named）

- 作成: `~~nsnew NS` は `NS` という名前の名前空間を作り、`(~NS const)` で参照可能にします。
- 定義: `~~nsdef NS const value` で `NS` に定義（副作用）。

例:

```
!{ ~~nsnew NS;
   ~~nsdef NS .Foo 42;           # symbol キー
   ~~nsdef NS "bar" 99;         # 文字列キー
   ~~println (~to_str ((~NS .Foo)))   # => 42
 };
```

## 無名（匿名）名前空間（Namespace value）

- 作成: `~~nsnew0` は新しい「名前空間値」を返します。これは 1 引数のビルトインで、`(~NS const)` と同様に動作します。
- 定義: `~~nsdefv ns const value` で、`ns` が「名前空間値」の場合に直接定義できます。
- セッター（代替）: `(~ns .__set)` は `(const val) -> ()` なセッタービルトインを返します。

例:

```
!{
  ns <- (~~nsnew0);
  (~~nsdefv ns .Foo 42);
  (~~nsdefv ns 42 "answer");
  ~~println (~to_str ((~ns .Foo)))  # => 42
};
```

セッター経由:

```
!{
  ns <- (~~nsnew0);
  ((~ns .__set) .Foo 42);
  ~~println (~to_str ((~ns .Foo)))  # => 42
};
```

## リテラル名前空間（不変な Namespace value）

- 構文: `{ const = value; ... }` は「不変（イミュータブル）」な名前空間値を生成します。
- 特徴: 作成時に決まったバインディングが固定され、後から変更できません。評価自体は遅延ですが、表（キー→値の対応）は不変です。
- 参照: 通常どおり `(~ns const)` で取得できます。
- 変更操作: `~~nsdefv` や `(~ns .__set)` による更新は許可されません（エラー）。

例:

```
!{
  ns <- { .Foo = 42; "bar" = 99; };
  ~~println (~to_str ((~ns .Foo)));        # => 42
  # 次のような更新はエラー（不変のため）:
  # (~~nsdefv ns .Foo 1)
  # ((~ns .__set) .Foo 1)
};
```

## 注意
- `nsdef`/`nsdefv` は効果（環境の更新）を伴います。`strict-effects` 有効時は `seq/chain` でトークンを通してください。
- `(~NS const)` の `const` は `int | str | constr(0-arity) | symbol(.name)`。
- 無名 NS は値として渡せるため、クロージャや局所スコープでの辞書用途に向きます。
- リテラル名前空間（`{ ... }`）は「非副作用」で不変です。作成後に再定義／更新はできません。

## 列挙（デバッグ）
- `~~nsMembers NS` / `~~nsMembers ns` は `list<const>` を返します（値は非評価）。
- 並び順の提案: 型順位（symbol < str < int < constr）→ 型内の辞書/数値順。

# Namespaces（仕様）

LazyScript の名前空間は「定数キー const から値への写像」を提供します。名前空間には2種類あります。

- Named namespace（レジストリ登録型）: 名前でグローバル登録された名前空間。
- Namespace value（無名/第一級値）: 値として渡せる名前空間。

本書では両者の共通仕様と相違点、作成・定義・参照・列挙の各操作を示します。

- 参照（共通）: `(~NS const)` で NS から値を取得（関数でも可）。
- シュガー: `~~sym` は `(~prelude sym)` の短縮。`LAZYSCRIPT_SUGAR_NS` で切替可能（デフォルト prelude）。

キー `const` は次のいずれかの定数型です: `int | str | constr(0-arity) | symbol`。シンボルは `.name` のリテラルで表し、インターンされ `==` 比較できます。
互換性のため、先頭が `.` の 0 引数コンストラクタ（例: `.Foo`）はキー位置では `symbol` と同一視されます。

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
- ミューテーション（環境更新）は効果です。`strict-effects` 有効時は `seq/chain` でトークンが必要です。
  - 対象: `nsnew`, `nsnew0`, `nsdef`, `nsdefv`, `(~ns .__set)`
- `(~NS const)` の `const` は `int | str | constr(0-arity) | symbol(.name)`。
- 無名 NS は値として渡せるため、クロージャや局所スコープでの辞書用途に向きます。
- リテラル名前空間（`{ ... }`）は非副作用です。現実装でもランタイムで更新操作をエラーにします（ソフトな不変）。将来的に型や検査での厳密化も検討します。

## 列挙（デバッグ）
- `~~nsMembers NS` / `~~nsMembers ns` は `list<const>` を返します（値は非評価）。
- 並び順（安定）: 型順位（symbol < string < int < constr）→ 型内はバイト列の辞書順。
  - 整数は 10 進文字列として比較されるため、辞書順になります（例: `10 < 2`）。
- 表示上の注意: `to_str` でのリスト表示では、文字列はクォート無しで出力されます。

例（列挙の順序）:

```
!{
  ns <- ((~prelude nsnew0));
  ((~prelude nsdefv) ~ns .a 1);
  ((~prelude nsdefv) ~ns .b 2);
  ((~prelude nsdefv) ~ns "aa" 3);
  ((~prelude nsdefv) ~ns "ab" 4);
  ((~prelude nsdefv) ~ns 2 5);
  ((~prelude nsdefv) ~ns 10 6);
  ((~prelude nsdefv) ~ns Foo 7);
  ((~prelude nsdefv) ~ns Bar 8);
  ~~println (~to_str ((~prelude nsMembers) ~ns));
};
-- 出力: [.a, .b, aa, ab, 10, 2, Bar, Foo]
```

strict-effects 有効時の例（トークンが必要）:

```
!{
  ns <- ((~prelude chain) ((~prelude nsnew0)) (\~x -> ~x));
  ((~prelude chain) ((~prelude nsdefv) ~ns .x 1) (\~_ -> ()));
  ((~prelude nsMembers) ~ns)
};
-- 出力: [.x]
```

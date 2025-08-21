# Namespaces（仕様）

LazyScript の名前空間は「シンボルキーから値への写像」を提供します。名前空間には2種類あります。

- Named namespace（レジストリ登録型）: 名前でグローバル登録された名前空間。
- Namespace value（無名/第一級値）: 値として渡せる名前空間。

本書では両者の共通仕様と相違点、作成・定義・参照・列挙の各操作を示します。

- 参照（共通）: `(~NS sym)` で NS から値を取得（関数でも可）。
- シュガー: `~~sym` は `(~prelude sym)` の短縮。`LAZYSCRIPT_SUGAR_NS` で切替可能（デフォルト prelude）。

キーは `.name` 形式のシンボルのみです。シンボルはインターンされ `==` 比較できます。文字列など他の型を指定すると構文エラーになります。

## 名前付き名前空間（Named）

- 作成: `~~nsnew NS` は `NS` という名前の名前空間を作り、`(~NS sym)` で参照可能にします。
- 定義: `~~nsdef NS sym value` で `NS` に定義（副作用）。

例:

```
!{ ~~nsnew NS;
   ~~nsdef NS .Foo 42;           # symbol キー
   ~~println (~to_str ((~NS .Foo)))   # => 42
 };
```

## 無名（匿名）名前空間（Namespace value）

- 作成: `~~nsnew0` は新しい「名前空間値」を返します。これは 1 引数のビルトインで、`(~NS sym)` と同様に動作します。
- 定義: `~~nsdefv ns sym value` で、`ns` が「名前空間値」の場合に直接定義できます。
- セッター（代替）: `(~ns .__set)` は `(sym val) -> ()` なセッタービルトインを返します。

例:

```
!{
  ~ns <- (~~nsnew0);
  (~~nsdefv ~ns .Foo 42);
  ~~println (~to_str ((~ns .Foo)))  # => 42
};
```

セッター経由:

```
!{
  ~ns <- (~~nsnew0);
  ((~ns .__set) .Foo 42);
  ~~println (~to_str ((~ns .Foo)))  # => 42
};
```

## リテラル名前空間（不変な Namespace value）

- 構文: `{ sym = value; ... }` は「不変（イミュータブル）」な名前空間値を生成します。
- 特徴: 作成時に決まったバインディングが固定され、後から変更できません。評価自体は遅延ですが、表（キー→値の対応）は不変です。
  - 参照: 通常どおり `(~ns sym)` で取得できます。
- メンバー定義 `.sym = expr` は同時に `~sym <- expr` を行い、同じ名前でローカル参照が利用できます。
- 変更操作: `~~nsdefv` や `(~ns .__set)` による更新は許可されません（エラー）。

例:

```
!{
  ~ns <- { .Foo = 42; .Bar = ~Foo; };
  ~~println (~to_str ((~ns .Foo)));        # => 42
  ~~println (~to_str ((~ns .Bar)));        # => 42
  # 次のような更新はエラー（不変のため）:
  # (~~nsdefv ~ns .Foo 1)
  # ((~ns .__set) .Foo 1)
};
```

## 注意
- ミューテーション（環境更新）は効果です。`strict-effects` 有効時は `seq/chain` でトークンが必要です。
  - 対象: `nsnew`, `nsnew0`, `nsdef`, `nsdefv`, `(~ns .__set)`
  - `(~NS sym)` の `sym` は `.name` 形式のシンボル。
- 無名 NS は値として渡せるため、クロージャや局所スコープでの辞書用途に向きます。
- リテラル名前空間（`{ ... }`）は非副作用です。現実装でもランタイムで更新操作をエラーにします（ソフトな不変）。将来的に型や検査での厳密化も検討します。
 - セッター互換: `(~ns .__set)` も許容され、`(~ns __set)` と同等です。
 - 誤用ヒント: `(~ns "__set")` や `(~ns ".__set")` は `#err "namespace: use (~ns __set) or (~ns .__set) for setter"` を返します。

## 列挙（デバッグ）
  - `~~nsMembers NS` / `~~nsMembers ns` は `list<symbol>` を返します（値は非評価）。
  - 並び順（安定）: シンボル名の辞書順。
- 表示上の注意: `to_str` でのリスト表示では、文字列はクォート無しで出力されます。

例（列挙の順序）:

  ```
  !{
    ~ns <- ((~prelude nsnew0));
    ~~nsdefv ~ns .a 1;
    ~~nsdefv ~ns .b 2;
    ~~nsdefv ~ns .aa 3;
    ~~nsdefv ~ns .ab 4;
    ~~println (~to_str ((~prelude nsMembers) ~ns));
  };
  -- 出力: [.a, .aa, .ab, .b]
  ```

strict-effects 有効時の例（トークンが必要）:

```
!{
  ~ns <- ((~prelude chain) ((~prelude nsnew0)) (\~x -> ~x));
  ((~prelude chain) (~~nsdefv ~ns .x 1) (\~_ -> ()));
  ((~prelude nsMembers) ~ns)
};
-- 出力: [.x]
```

## nslit デバッグログ

環境変数 `LS_NS_LOG_NSLIT` を設定すると、名前空間リテラル（nslit）の評価過程を stderr にログ出力します。値が空文字列または `0` 以外であれば有効になります（既定はオフ）。

### 例

```bash
LS_NS_LOG_NSLIT=1 ./src/lazyscript -e '!{ ~ns <- { .Foo = 1; }; (~ns .Foo); };'
```

出力例:

```
DBG nslit begin argc=2
DBG nslit key[0] eval
DBG nslit key=S.Foo
DBG nslit val[0] eval
DBG nslit put type=3
DBG nslit end ns=0x... map=0x...
```


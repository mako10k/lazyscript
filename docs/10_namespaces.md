# Namespaces（仕様）

LazyScript の名前空間は「シンボルキー → 値」の写像です。二つの形があります。

- 名前付き Namespace（登録型）
- 名前空間値（第一級値）

どちらも参照は関数適用で行います: `(~NS .Key)`。糖衣 `~~sym` は `(~prelude sym)` の短縮です。

キールール
- キーは必ず `.Name` 形式のシンボル値です（文字列不可）。
- シンボルはインターンされ、`==` 比較は O(1)。
- 単引用 `'Name'` は字句としては存在しますが、キーには使わず `.Name` を使います。

## 名前付き Namespace

- 作成: `~~nsnew NS`
- 定義: `~~nsdef NS .Key value`

例:

```
!{ ~~nsnew NS;
   ~~nsdef NS .Foo 42;
   ~~println (~to_str ((~NS .Foo)))
};
```

ドット連鎖（構文の結合則）:

```
!{
  ns <- { .A = { .B = 1 } };
  ~~println (~to_str ((~ns .A).B));
};
```

## 名前空間値（無名）

- 作成: `~~nsnew0` → 新しい NS 値
- 定義: `~~nsdefv ns .Key value`
- セッター: `(~ns .__set)` は `(sym val) -> ()`

例:

```
!{
  ns <- (~~nsnew0);
  (~~nsdefv ns .Foo 42);
  ~~println (~to_str ((~ns .Foo)))
};
```

セッター経由:

```
!{
  ns <- (~~nsnew0);
  ((~ns .__set) .Foo 42);
  ~~println (~to_str ((~ns .Foo)))
};
```

## リテラル Namespace（不変）

- リテラル: `{ .A = 1; .B = 2 }` は不変な NS 値。
- メンバーは「シンボル = 式」の列のみを許可します（ns 内でのローカル束縛 `p <- e` は廃止）。
- 相互参照が必要な場合は、外側で `let`/束縛を行うか、Prelude 経由で値を組み立ててから埋め込んでください。

## 列挙（nsMembers）

- `~~nsMembers NS` / `~~nsMembers ns` → `list<symbol>`（値は非評価）
- 並び順はシンボル名の辞書順（安定）

例:

```
!{
  ns <- (~~nsnew0);
  ~~nsdefv ~ns .a 1;
  ~~nsdefv ~ns .b 2;
  ~~nsdefv ~ns .aa 3;
  ~~nsdefv ~ns .ab 4;
  ~~println (~to_str (~~nsMembers ~ns));
};
-- 出力: [.a, .aa, .ab, .b]
```

## 効果と安全

- 環境更新（nsnew/nsdef/...）は効果です。厳格モードでは `chain/seq` 等で順序を示してください。

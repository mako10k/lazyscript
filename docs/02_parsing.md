# 構文解析 (Parsing)

構文定義は `src/parser/parser.y` (Bison) にあり、純粋パーサで位置情報を扱います。トップレベル開始記号は `prog` です。

## トップレベル

- `prog: expr ';'` — セミコロンで 1 式を終了。`lsprog_new(expr)` を生成。

## 式 (expr)

演算子優先順位は下に行くほど強くなります。

1. `expr1`: 右結合の選択 `|`
   - `expr2 '|' expr1` → `lsexpr_new_choice(lsechoice_new(lhs, rhs))`
2. `expr2`: 右結合の `:`（cons 構築子）を `econs` で扱う
   - `expr1 ':' expr3` → 代数データ `":"` コンストラクタ (引数2)
3. `expr3`: 関数適用 `eappl`
   - `efact expr5` → 初回引数で `lseappl_new`
   - `eappl expr5` → 追加引数で `lseappl_add_arg`
4. `expr4`: 代数データリテラル `ealge`
   - `LSTSYMBOL expr5` → `lsealge_new(sym, 1, &arg)`
   - `ealge expr5` → `lsealge_add_arg`
5. `expr5`: 原子
   - `efact` | `LSTSYMBOL`(引数 0 の代数データ) → `lsexpr_new_alge(lsealge_new(sym,0,NULL))`

### 原子 (efact)
- 整数: `LSTINT` → `lsexpr_new_int`
- 文字列: `LSTSTR` → `lsexpr_new_str`
- タプル: `etuple` — `,` コンストラクタ。要素 1 個ならそのまま値、複数なら `alge` 化
- リスト: `elist` — `[]` コンストラクタ
- クロージャ: `closure` — 本体式と束縛を持つ
- 参照: `'~' LSTSYMBOL` → `lsexpr_new_ref(lsref_new(sym, loc))`
- ラムダ: `elambda`
- ブロック: `'{' expr '}'` → 式そのまま

補足（シンタックスシュガー）:
- `~~sym` は `(~<ns> sym)` に展開（既定 ns は `prelude`）。
- `!sym` は `(~<ns> .env .sym)` に展開（環境操作 API: `!require` など）。
- `!{ ... }` は `(~<ns> chain/bind/return)` 連鎖に展開（予約語なしの do 記法）。
- `<ns>` は CLI `--sugar-namespace <ns>` または環境変数 `LAZYSCRIPT_SUGAR_NS` で切替可能。
詳しくは `docs/05_syntax_sugar.md` を参照。

### タプル・リスト
- `etuple`:
  - `()` → `","` コンストラクタ(0)
  - `(e1, e2, ...)` → `","` に列挙
- `elist`:
  - `[]` → `"[]"` コンストラクタ(0)
  - `[e1, e2, ...]` → `"[]"` に列挙

### 関数適用 (eappl)
- `efact expr5` で 1 引数適用。以降 `eappl expr5` で可変長に拡張。

プリティプリンタの出力規則（ドット連結）

- 先頭引数がドット記号（`.sym`）のとき、関数名に連結してメンバアクセスとして出力する。
- いったん連結が始まったら、後続引数がドット記号である限り連結を継続する。ドット以外の引数が現れたら連結を終了する。
- この「連結」は通常の関数適用よりも高い優先順位を持つ。したがって不要な括弧は省かれる。
- 空白や改行の有無には依存しない（字句上の隣接性ヒューリスティクスは採用しない）。

例（いずれも最終出力の例）

- `~a ~b.c` と `~a (~b.c)` は同じく `~a ~b.c`
- `~a ~b.c.d .e` は `~a ~b.c.d .e`（`.e` は別引数として扱われる）
- `~a ~b.c.d.e .f.g` は `~a ~b.c.d.e.f.g`
- `~a (~b.c).d .e` は `~a ~b.c.d.e`

## ラムダ・パターン・束縛

- ラムダ: `elambda: '\\' pat '->' expr3` → `lselambda_new(pat, body)`
- パターン `pat` は以下:
  - 基本: `pat3`
  - データ: `palge` を `lspat_new_alge` でラップ
  - cons: `pat ':' pat`
- `pat3` の要素:
  - タプル `ptuple`/リスト `plist`
  - 整数 `LSTINT`、文字列 `LSTSTR`
  - 参照 `pref` → `lspat_new_ref`
  - as パターン `pref '@' pat3` → `lspat_new_as(lspas_new(pref, pat3))`
- `pref`: `'~' LSTSYMBOL` → `lsref_new`
- 束縛リスト `bind`: `pat '=' expr` の列を `;` 区切りで。
- クロージャ: `( expr bind )` → `lseclosure_new(expr, binds)`

## 実装ノート
- Bison のスタック確保は GC ラッパ (`lsmalloc/lsfree`) を使用。
- 位置型は `lsloc_t`。`%locations`, `YYLLOC_DEFAULT` で設定。
- `parse.error verbose`, `parse.lac full` を有効化。

## 予約語なしで書く「let」実務ガイド

予約語を増やさずにローカル束縛（let 相当）を表現する代表的な方法は次の3つです。いずれも既存の構文だけで記述できます。

1) ラムダ適用にデシュガー（基本形）
- 形: `let p = e1 in e2` ≒ `(\ p -> e2) e1`
- 例: `(\ ~x -> ~add ~x 1) 41`
- 複数束縛はパターンで対応: `(\ ( ~x, ~y ) -> ~add ~x ~y) (1, 2)`

2) クロージャ + バインド列（where/let-in 風で読みやすい）
- 構文: `( 本体式 ; pat1 = expr1; pat2 = expr2 )`
- 例: `( ~add ~x ~y; ~x = 1; ~y = 2 )` → `3`
- `pat` は通常のパターン（`~x`、`( ~x, ~y )`、コンストラクタ等）が使えます。
- 内部的には 1) のラムダ適用（あるいは ANF の let）に落とせます。

3) chain/return による do 的スタイル（効果順序を明確化）
- 形: `chain e (\ ~x -> body)`、純粋値は `return v`
- 例: `chain 1 (\ ~x -> chain 2 (\ ~y -> ~add ~x ~y))`
- `--strict-effects` でも順序が明確で安全です。

推奨
- 可読性重視なら 2) を表記として使い、コンパイラ内部では 1) にデシュガー（ANF 化）すると単純です。
- 効果や順序の規律が重要な箇所のみ 3) を併用してください。

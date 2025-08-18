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

### タプル・リスト
- `etuple`:
  - `()` → `","` コンストラクタ(0)
  - `(e1, e2, ...)` → `","` に列挙
- `elist`:
  - `[]` → `"[]"` コンストラクタ(0)
  - `[e1, e2, ...]` → `"[]"` に列挙

### 関数適用 (eappl)
- `efact expr5` で 1 引数適用。以降 `eappl expr5` で可変長に拡張。

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

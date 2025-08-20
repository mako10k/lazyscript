# Builtins, Prelude, and Namespace Introspection

この文書は、ビルトイン名前空間ローディング（`~~builtin`）、暗黙プリリュード注入、デバッグ用名前空間列挙（`~~nsMembers`）の仕様をまとめます。現状の実装仕様に基づき記述し、将来の拡張余地も明記します。

## 目的と方針
- グローバルに `~add` のようなビルトイン直載せを避け、名前空間へ整理する。
- 外部共有ライブラリ（.so）をモジュール単位で動的ロードして提供する。
- プリリュードはその仕組みの一例として実行開始時に暗黙注入する。
- デバッグ容易化のため、名前空間メンバーを列挙できるプリミティブを提供する。

## `~~builtin`: ビルトイン名前空間ローダー

- 構文: `~~builtin "basename"` → レコード値（名前空間）
- 例: `~arith = ~~builtin "arith"; ~arith add 1 2;`
- パス解決（優先順）:
  1) `LAZYSCRIPT_BUILTIN_PATH`（コロン区切り）
  2) 実行ファイル相対 `builtins/` または `plugins/`
  3) `/usr/local/lib/lazyscript:/usr/lib/lazyscript`
  - 絶対パスも可（`"/abs/path/foo.so"`）。
- キャッシュ: `basename` ごとに 1 回ロード。`dlclose` はしない（プロセス終了まで有効）。
- 共有ライブラリ ABI（提案）:
  - `dlsym("lazyscript_builtin_init")` を呼び、モジュール記述子を取得。
  - 記述子: `abi_version`, `module_name`, `module_version`,
    `functions: [{ name, arity, fn, effects?, doc? }]`（`fn` は `lstbuiltin_func_t` 互換）。
- エラー: 見つからない/ABI不一致/重複名 → `#err`（理由を含めて報告）。
- 並行性: 初回ロードと登録はミューテックス保護。
- トレース: 擬似 loc `builtin:basename#name` を JSONL に記録（スタックにも反映）。

## Prelude: 暗黙注入されるビルトイン名前空間

- 実行開始時、ルート環境に暗黙注入: `~prelude = <BuiltinLoader> "prelude"`。
- `<BuiltinLoader>` は実行環境が提供（ユーザは記述不要）。
- 糖衣 `~~sym` は既定で `(~prelude sym)` に展開（CLI/ENV で上書き可）。
- 構成案: `--no-prelude` / `LAZYSCRIPT_NO_PRELUDE=1` で無効化、`LAZYSCRIPT_PRELUDE` で別モジュール指定。
- 互換: 既存のトップレベル `~add` などは当面維持、推奨は `~prelude` 経由。

## `~~nsMembers`: 名前空間メンバー列挙（デバッグ用）

- 構文: `~~nsMembers nsValue` → `list<const>`
- 振る舞い:
  - 名前空間（名前付き/無名/ビルトイン含む）の「キー（const）」を列挙。
  - キーは評価不要な定数（const）として保持され、値は評価しない（非評価）。
  - 返却順は安定ソート。型順位（symbol < string < int < constr）→ 型内は辞書順。
- エラー: 非NS → `#err "nsMembers: not a namespace"`。
- トレース: 擬似 loc `introspect:nsMembers` を JSONL に記録。

## 将来拡張
- `~~nsHas`, `~~nsDescribe`（メタ列挙）、ビルトイン署名検証とバージョンピン留め（`arith@1`）、
  効果型連携（`effects` メタの型検査反映）、署名/サンドボックス等の強化。

## 備考（シンボル vs 文字列）
- `.symbol` リテラル（例: `.name`）は実装済み。インターンされ `==` 比較が O(1)。

## シンボルリテラル `.symbol` と名前空間キーの const 化

糖衣は導入せず、`.name` は「シンボル型（Symbol）のリテラル」を表します。シンボルはインターンされ、`==` による同値比較が O(1) で可能です。

- 型: `Symbol`（例: `.concat`, `.builtin`）
- リテラル規則（概要）: `'.' IDENT` を 1 トークンとして読み取る（浮動小数点は未採用のため衝突なし）。
- インターン: 実行系で一意化（同名は同一値）。`to_string` 表示は `".name"` 風を想定。

### 名前空間の定義と参照

- レコード（名前空間）定義は「定数キー const のマップ」に緩和:
  - `const := int | str | constr(0-arity) | symbol`
  - 例: `{ .add = value; "println" = value; 42 = value; :Unit = value; }`
- 参照は「関数適用」スタイルで統一:
  - `(~NS const)` で対応する値を取得（キーは `==` で一致判定）。
  - 例: `~list = (~prelude .builtin) "list"; (~list .concat) [1,2] [3,4]`
- 注意: 将来、パターンをキーに拡張する余地はあるが、`nsMembers` が取得不能になるため現段階は `const` に限定。

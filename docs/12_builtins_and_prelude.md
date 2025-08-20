# Builtins, Prelude, and Namespace Introspection

この文書は、ビルトイン名前空間ローディング（`~~builtin`）、暗黙プリリュード注入、デバッグ用名前空間列挙（`~~nsMembers`）の仕様をまとめます。実装前提の合意仕様であり、将来の拡張余地も明記します。

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

- 構文: `~~nsMembers nsValue` → `list<string>`
- 振る舞い:
  - レコード/ビルトインNS/`~prelude` のメンバー名を列挙。
  - 値は評価しない（非評価）。辞書順（ASCII）で安定返却。
- エラー: 非NS → `#err "nsMembers: not a namespace"`。ビルトインNS破損は理由付き `#err`。
- トレース: 擬似 loc `introspect:nsMembers` を JSONL に記録。

## 将来拡張
- `~~nsHas`, `~~nsDescribe`（メタ列挙）、ビルトイン署名検証とバージョンピン留め（`arith@1`）、
  効果型連携（`effects` メタの型検査反映）、署名/サンドボックス等の強化。

## 備考（シンボル vs 文字列）
- シンボル型（例 `:name`）導入案は保留。現状は文字列キーで運用し、将来移行可能とする。

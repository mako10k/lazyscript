# Builtins と Prelude（最新仕様）

この文書は「ビルトインの提供方法」と「Prelude の構造」を最新アーキテクチャに沿って説明します。

要点
- コアビルトインは外部プラグイン `core_builtins.so` で提供します。
- `~builtins` と `~internal` は Prelude 評価中のみホストから注入され、ユーザコードからは見えません。
- `~prelude` は「値」です。起動時に `requirePure("lib/Prelude.ls")` の結果で束縛されます。
- ユーザは `~~sym`（純粋 API）と `!sym`（環境 API）経由で Prelude を使います。

## 提供コンポーネント

- core_builtins プラグイン（純粋/基本操作）
  - to_str, print, println, seq, add, sub, lt, eq
  - 制御系: chain, bind, return
  - 名前空間: nsMembers, nsSelf
- Prelude.ls（ユーザ面の窓口）
  - 上記の純粋 API を再エクスポート（`~~sym` で参照可能）。
  - `.env` 名前空間を持ち、環境操作 API をここから露出。
- internal（環境 API 実体）
  - require, requirePure, import, withImport, def, nsnew/nsdef/nsnew0/nsdefv など。
  - これらは `~internal` として Prelude 評価時のみ利用でき、`Prelude.ls` 内の `.env` にブリッジされます。

## スコープと可視性

- ホストは Prelude を評価する直前に一時的に `~builtins` と `~internal` を注入します。
- Prelude の評価が終わると、`~prelude` にその値を束縛し、`~builtins/~internal` は破棄します。
- 実行時のユーザコードからは `~prelude` のみが見えます（値）。

## 使い方（ユーザ視点）

- 純粋 API: `~~sym` → `(~prelude sym)` に展開
  - 例: `((~~println) "hello")`
- 環境 API: `!sym` → `(~prelude .env .sym)` に展開
  - 例: `!require "lib/Foo.ls"`
- do ブロック: `!{ ... }` は `chain/bind/return` で順序を明示
  - 例: `!{ !require "lib.x"; ~~println "ok" }`

## 注意点

- 互換のため古い記述 `~add` 等が残っていても、推奨は `~~add` または `!` 系の使用です。
- bash の `!` 履歴展開に注意。`-e` で与える際はクォートしてください。

## 名前空間のイントロスペクション

- `~~nsMembers ns` は `list<symbol>` を返します（値は非評価）。
- `~~nsSelf` はレコード内から自分自身を参照する際に使えます（`{ .A = (~nsSelf .B); .B = 1 }` など）。

## 参考: 実装の流れ（ホスト側）

1) `~builtins` と `~internal` を一時注入し、`Prelude.ls` を評価
2) 評価結果を `~prelude` に束縛（値）
3) `~builtins/~internal` を除去（ユーザには見えない）

## 代表的なエクスポート一覧（Prelude）

- 純粋: println, print, to_str, eq, lt, add, sub, seq, chain, bind, return, nsMembers, nsSelf
- 環境: `.env` 配下に require, requirePure, import, withImport, def, nsnew/nsdef/nsnew0/nsdefv など


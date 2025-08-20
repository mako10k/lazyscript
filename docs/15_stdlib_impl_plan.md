# 標準ライブラリ 実装計画（MVP → 拡張）

目的: 無名名前空間リテラルで自己完結なモジュールを提供し、`~~require` で読み込んで使える実用APIを段階的に整備する。

## 方針
- モジュールは基本「無名名前空間リテラル `{ ... }`」で実装する（不変）。
- 公開APIはシンボルキーでエクスポート。`nsMembers` に列挙される。
- 作用のある関数は strict-effects に準拠し、必要に応じて `chain` を併用する設計例を docs に示す。
- 依存は最小限。相互依存は `~~require` のみ。

## MVP セット（スプリント1）
- lib/list.ls: list 基本（cons, nil, map, filter, foldl, foldr, length, append, flatMap, reverse, toString）
- lib/option.ls: Option { None | Some a } と map/flatMap/getOrElse
- lib/result.ls: Result { Ok a | Err e } と map/flatMap/withDefault
- lib/string.ls: length, substring, indexOf, split, join（可能な範囲で）
- lib/ns.ls: nsMembers 再エクスポート + ユーティリティ（nsHas, nsGetOr）

## スプリント2
- lib/set.ls / lib/map.ls: シンボルキーの ns ラップ or 簡易実装
- lib/array.ls: 可変配列（効果注意） or リストラッパ
- lib/format.ls: 文字列整形、toString 合成

## API デザインメモ
- map/filter 系は第1引数に関数（lambda）を取り、続いてコレクション。
  - 例: `(~List .map) (\~x -> ~x+1) [1,2]` のように `(~NS sym)` で参照。
- 失敗は `Result` を優先。例外は最小化。

## テスト方針
- 各モジュールに対して `.ls/.out` を追加。`~~require "lib/xxx.ls"` で読み、代表ケースを確認。
- nsMembers ベースの公開関数確認（順序は仕様依存）。

## ロールアウト
- ブランチ feature/stdlib-init で段階的に実装→テスト→コミット。
- まとまりごとに PR を作成（list→option/result→string→ns の順）。

# Standard Library Plan (Initial)

標準ライブラリ（LazyScript modules）の初期案。各モジュールは「無名名前空間でレコードを返す」`lib/*.ls` を基本とし、`~~require "module"` で読み込んで利用します。

## 方針
- 既存ビルトイン（`~add`, `println` など）を最小限ラップし、名前空間経由で提供。
- 段階的に関数群を拡充。まずは日常ユースの小粒ユーティリティに集中。
- 互換維持しつつ、明示の名前空間利用（別名束縛）を推奨。

## 初期モジュール（提案）
- math: 算術・数値ユーティリティ
- cmp: 比較・最小/最大
- bool: 論理演算
- string: 文字列操作
- list: リスト操作（既存 `lib/list.ls` を拡充）
- option: Optional 値操作
- result: Ok/Err 操作
- io: 出力などの副作用系
- func: 関数コンビネータ
- tuple: ペア/タプル操作

将来候補: ns, map/dict, set, regex, json, time, file, random など

## エクスポート例（抜粋）
- math: `add, sub, mul, div, mod, neg, abs, sign, clamp, inc, dec, sum, product`
- cmp: `eq, ne, lt, lte, gt, gte, min, max, compare`
- bool: `and, or, not, xor, all, any`
- string: `length, concat, repeat, trim, trimStart, trimEnd, toUpper, toLower, startsWith, endsWith, includes, split, join, toString`
- list: `length, head, tail, last, init, take, drop, slice, append, concat, flatten, uniq, map, filter, foldl, foldr, find, any, all, sum, product, joinStr`
- option: `some, none, isSome, isNone, withDefault, map, andThen, getOr`
- result: `ok, err, isOk, isErr, withDefault, map, mapErr, andThen, unwrapOr`
- io: `println, print, eprintln?, debug`
- func: `id, const, flip, compose, pipe, tap`
- tuple: `pair, fst, snd, swap`

## 配置と利用
- 各ファイル: `lib/<module>.ls` がレコードを返す。
- 例: `~math = ~~require "math"; math.add 1 2;`
- 別名: `~add = (~math add)` のように alias を用意可能。

## テスト方針
- 各モジュール 1〜3 ケースの最小テストを `test/` に追加。
- 例: `~~require "math"` で代表関数の検証。

## 今後
- まずは alias ベース（ビルトイン委譲）で雛形を追加。
- 実装が落ち着いたらドキュメントに使用例とAPI表を整備。

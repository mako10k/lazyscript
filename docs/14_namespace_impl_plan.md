# 名前空間リニューアル 実装計画（symbols + const-keyed namespaces）

この文書は、.symbol リテラルと const キー（int/str/0-arity constr/symbol）による名前空間の統一仕様を段階的に実装するための計画です。

## 目的と範囲
- Symbol 型（.name リテラル）の導入とインターン機構
- const キーでの参照/定義（(~NS const), ~~nsdef/~~nsdefv）
- 無名 NS（可変）とリテラル NS（不変）の併存
- nsMembers の `list<const>` 返却と安定ソート
- 効果（nsdef/nsdefv/__set）の扱いを既存ポリシーに統合
   - strict-effects 連携: ns のミューテーション（nsnew/nsdef/nsdefv/__set）は効果としてガードし、pure 文脈では明示エラー。
- 互換性維持（可能な範囲で旧入力も許容 or 明示エラー）

非目標（本計画では扱わない）
- ドット記法のメンバーアクセス糖衣
- 複数スレッド下でのロック最適化（最初は単純実装）

## フェーズ分割
1. 型・表示の基盤（最小の差分）
   - Symbol 型の追加（lstypes.h/…）。等価性（ポインタ/ID 比較）と to_string（".name"）
   - インターンテーブル（グローバル）実装（ハッシュ+永続文字域）。解放はプロセス終了まで維持で可。
   - テスト: `.foo == .foo`, `.foo != .bar`, to_string, パース/プリント往復。

2. const キーの表現と比較
   - const 判定/比較ユーティリティ（symbol < str < int < constr の順序）。
   - 0-arity 構築子の const 化（既存の型情報から識別）。
   - テスト: 比較順序、安定ソート。

3. 名前空間内部表現の拡張
   - NS マップを「const -> value」に統一（現在の実装を拡張 or 新型を追加）。
   - 可変/不変フラグとエラーハンドリング（不変 NS への set はエラー）。
   - Named レジストリ（名前→NS）を既存のまま利用（中身は同一 NS 型）。
   - テスト: 基本 put/get、衝突と上書き、エラー系（不変に対する set）。

4. ビルトインの更新
   - ~~nsnew: 名前付き NS 作成（可変）。
   - ~~nsdef: 名前付き NS に const で定義（効果）。
   - ~~nsnew0: 無名 NS 値（可変）を返す。
   - ~~nsdefv: 無名 NS に const で定義（効果）。
   - (~ns .__set): セッタ（(const val)->()）。不変ならエラー。
   - ~~nsMembers: list<const> 返却（安定ソート）。
   - テスト: 代表ケース＋厳密効果モードでの動作。

5. パーサ/レキサ拡張
   - .symbol リテラルのトークン追加（例: SYM_LIT）。
   - const 非終端の導入（int/str/constr0/symbol）。
   - (~NS const) の経路と nsdef/nsdefv の引数で const を受理。
   - リテラル NS `{ const = value; ... }` のキー側に symbol を許容。
   - テスト: 構文とエラーメッセージ、既存との非衝突確認。

6. 効果システムとの統合
   - nsnew/nsdef/nsdefv/__set を効果として宣言し、strict-effects で要トークン（seq/chain）に。
   - 実装: lsbuiltin_nsnew/lsbuiltin_nsdef/lsbuiltin_nsdefv/namespace.__set に ls_effects_allowed() ガードとエラーメッセージを追加。
   - テスト: strict-effects on/off の差異（on で失敗、chain/seq で成功）。

7. 互換性と移行
   - 旧インターフェイス（もしシンボル表現が存在する場合）を const へブリッジ、または明示エラー+メッセージ。
   - ドキュメントと CHANGELOG 更新。

## 影響ファイル（予想）
- レキサ/パーサ: `src/parser/lexer.l`, `src/parser/parser.y`
- 型/値: `src/lstypes.h`, `src/.../value*.c`（実装構造に応じて）
- 名前空間: `src/runtime/...` または `src/builtins/...`（現構成に追従）
- ビルトイン定義: `src/builtins/*.c`（nsnew/nsdef/nsnew0/nsdefv/nsMembers/__set）
- 効果/型検査: `src/.../effects*.c`（該当があれば）
- ドキュメント: `docs/10_namespaces.md`, `docs/12_builtins_and_prelude.md`
- テスト: `test/t3x_*` 追加

※ 正確なファイルは探索のうえで調整。

## データ構造メモ
- Symbol: interned { id:uint32, name:char* }。比較は id。
- Const: tagged union { SYMBOL, STR, INT, CONSTR0 }。総合比較関数 cmp_const。
- Namespace: { map<const,value>, mutable:bool }。map はハッシュ（const のハッシュ実装要）。

## エラーモデル
- 不変 NS に対する set/nsdefv → ランタイムエラー（メッセージ: immutable namespace）。
- キーが const でない → 型エラー（const expected）。
- 名前未登録（~~nsdef の対象 NS が無い）→ ランタイムエラー（unknown namespace）。
 - strict-effects 有効時のミューテーション（nsnew/nsdef/nsdefv/__set）→ 純粋文脈ではエラー（"effect used in pure context (enable seq/chain)"）。

## リスク/対策
- レキサ衝突（.symbol）: ドットの既存用途を確認し、先読みで SYM_LIT を優先。
- インターンのメモリリーク: プロセス常駐で許容。将来 GC に統合。
- パフォーマンス: ハッシュ化と比較を軽量に（symbol は id ベース）。
- 既存コードへの影響: 段階導入＋テスト駆動、フェーズ毎に green を維持。

## テスト計画（抜粋）
- t34_symbol_basic.ls: `.a == .a`, `.a != .b`, to_string
- t35_const_order.ls: nsMembers 並び、比較順序の検証
- t36_ns_mutable.ls: nsnew0/nsdefv/(__set) の成功系
- t37_ns_immutable.ls: `{ ... }` への更新でエラー
- t38_ns_named.ls: nsnew/nsdef と参照
- t39_ns_effects.ls: strict-effects on/off での差異
- t40_ns_mixed_keys.ls: int/str/symbol/constr0 混在

## マイルストーン/所要目安
- P1（1〜2日）: フェーズ1-2 + 基本テスト
- P2（2〜3日）: フェーズ3-4 + テスト
- P3（1〜2日）: フェーズ5-6 + 互換・ドキュメント
- バッファ（1日）: 調整とリグレッション

## ロールアウト
- feature ブランチでフェーズ毎にコミット/テスト
- PR を小さく分割（例: symbols 基盤 → const/NS → builtins → parser/effects）
- ドキュメント更新を各 PR に同梱

---
補足: リテラル名前空間は「非副作用・不変」、nsdef/nsdefv は「副作用・可変」です。詳細は `docs/10_namespaces.md` を参照。

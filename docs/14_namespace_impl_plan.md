# 名前空間リニューアル 実装計画（symbols-only／匿名NSへ集約）

この文書は、.symbol リテラルとシンボルキーによる名前空間の統一仕様を段階的に実装するための計画に、
「(1) グローバル名前付き可変NSの廃止」と「(2) 匿名可変NS／(3) 匿名不変NSへの集約」を加えた最新版です。

要点
- 名前空間は「シンボル→値」の辞書を保持する第一級値。内部はマップを指す参照を持ち、適用（`(~ns .key)`）でディスパッチします（実装はアリティ1のビルトイン／ディスパッチャ）。
- 現状の種類: (1) グローバル名前付き可変NS, (2) 匿名可変NS, (3) 匿名不変NS。
- 方針: (1) を段階的に廃止。（2）（3）だけを第一級値として提供。グローバル変数やレジストリから独立した値（データ／参照構造）に統一。

## 目的と範囲
- Symbol 型（.name リテラル）の導入とインターン機構
- const キーでの参照/定義（(~NS const), ~~nsdef/~~nsdefv）
- 無名 NS（可変）とリテラル NS（不変）の併存
- nsMembers の `list<symbol>` 返却と安定ソート
- 効果（nsdef/nsdefv/__set）の扱いを既存ポリシーに統合
   - strict-effects 連携: ns のミューテーション（nsnew/nsdef/nsdefv/__set）は効果としてガードし、pure 文脈では明示エラー。
- 互換性維持（可能な範囲で旧入力も許容 or 明示エラー）

非目標（追加）
- 旧「グローバル名前付き可変NS」(1) の機能拡張（削除対象）

ドット記法の優先順位は構文章（02/05/10）側で扱います。

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
   - NS マップを「const -> value」に統一（mutable/immutable を同一構造+フラグで）。
   - 不変 NS への set はエラー。
   - 旧「名前→NS」レジストリは廃止。以降は NS 値を受け渡し（第一級流通）。
   - テスト: 基本 put/get、衝突と上書き、エラー系（不変 set）。

4. ビルトインの更新
   - 廃止: `nsnew`/`nsdef`（名前付き＝グローバル登録）。
   - 継続: `nsnew0`（匿名可変NSを返す）, `nsdefv`（匿名NSへ定義）。
   - (~ns .__set): セッタ（(const val)->()）。不変ならエラー。
   - ~~nsMembers: list<symbol> 返却（安定ソート）。
   - テスト: 代表ケース＋厳密効果モードでの動作。

5. パーサ/レキサ拡張
   - .symbol リテラルのトークン追加（例: SYM_LIT）。
   - const 非終端の導入（int/str/constr0/symbol）。
   - (~NS const) の経路と nsdef/nsdefv の引数で const を受理。
   - リテラル NS `{ const = value; ... }` のキー側に symbol を許容。
   - テスト: 構文とエラーメッセージ、既存との非衝突確認。

6. 効果システムとの統合（対応済み）
   - nsnew/nsdef/nsdefv/__set を効果として宣言し、strict-effects で要トークン（seq/chain）に。
   - 実装: lsbuiltin_nsnew/lsbuiltin_nsdef/lsbuiltin_nsdefv/namespace.__set に ls_effects_allowed() ガードとエラーメッセージを追加。
   - テスト: strict-effects on/off の差異（on で失敗、chain/seq で成功）。

7. 互換性と移行（更新）
    - 非推奨フェーズ
       - `nsnew`/`nsdef`（名前付き）呼び出しで警告を出す。将来の削除時期を明示。
       - 実行は継続（即時破壊は避ける）。
    - ブリッジ/置換ガイド
       - 1行置換: 名前付き → 匿名NS値＋受け渡し
          - Before: `~~nsnew 'Foo; ~~nsdef 'Foo .Bar v;`
          - After: `!{ ~ns <- { .Bar = v }; ... }`
       - 連鎖定義
          - Before: `~~nsnew 'M; ~~nsdef 'M .A a; ~~nsdef 'M .B b; use 'M ...`
          - After: `!{ ~ns <- { .A = a; .B = b }; use ~ns ... }`
       - 値としての受け渡し
          - Before: 名前で共有
          - After: 関数引数/戻り値で NS 値を渡す
    - 完全撤廃
       - 期限後 `nsnew`/`nsdef` を削除。エラー文言で置換例を提示。

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
- 第一級値: NS は 1引数ビルトインのディスパッチャとして表現（`(~ns .key)` で検索、`(~ns __set)` で更新）。

## エラーモデル（実装反映）
- 不変 NS に対する set/nsdefv → `#err "namespace: immutable"` / `#err "nsdefv: immutable namespace"`。
- キーが const でない → `#err "...: const expected"`。
- 名前付き系（旧）の未登録 → 非推奨期は警告、撤廃後は `#err "nsdef: named namespaces are removed; use { ... } or nsnew0+nsdefv"`。
- strict-effects 有効時のミューテーション（nsnew/nsdef/nsdefv/__set）→ 純粋文脈では stderr にメッセージを出し `null`（実装互換）。

## リスク/対策
- レキサ衝突（.symbol）: ドットの既存用途を確認し、先読みで SYM_LIT を優先。
- インターンのメモリリーク: プロセス常駐で許容。将来 GC に統合。
- パフォーマンス: ハッシュ化と比較を軽量に（symbol は id ベース）。
- 既存コードへの影響: 段階導入＋テスト駆動、フェーズ毎に green を維持。

## テスト計画（実装対応のマッピング）
- t35–t36: const キー混在と nsMembers 並び（実装済みテストにより検証済み）
- t39–t41: strict-effects の on/off と chain 経路（実装済み）
- t42–t44: リテラル/名前付き NS の不変更新禁止（実装済み）
- 追加予定: 未登録 NS への nsdef エラー（`nsdef: unknown namespace`）のテスト

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

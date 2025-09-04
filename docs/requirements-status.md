# NSLIT コア実装と関連要件: 状態一覧

最終更新: 2025-09-04

## 要件一覧（抜粋）

- NSLIT は prelude 経由に依存せず、コアとして評価されること
  - AST `{ ... }` は (~prelude nslit$N) に落とさない
  - 値は遅延（メンバーは必要時に評価）
  - nsSelf は NSLIT 評価中のみ有効
- ~~sym は (~prelude .sym) に等価、!sym は (~prelude .env .sym)
- ~prelude は Prelude.ls の評価結果の値であり、plugin/dispatch でない
- 未束縛変数の診断（定義位置を表示）
- include の字句レイヤー実装と循環検出、チェーンを yyerror 経由で内→外順に表示
- エラー時の終了ステータスを非0に統一（フラグで切替可）
- List.ls の安全性（無限ループ/セグフォ回避）

## 実装状態

- NSLIT コア評価
  - 状態: 実装
  - 変更点:
    - `src/thunk/nslit_eval.[ch]` を追加（コア評価関数の登録・呼出し）
    - `src/lazyscript.c` で `ls_register_nslit_eval(lsbuiltin_nslit, NULL)` を実行
    - `src/thunk/thunk.c` の LSETYPE_NSLIT 分岐を、prelude 経由から `ls_call_nslit_eval` 呼び出しに変更
    - `lib/Prelude.ls` から `.nslit$*` のプレリュード委譲を削除
  - メモ: 依存層を守りつつ、AST→thunk でコア関数へ到達

- ~~ / ! の糖衣
  - 状態: 実装済（レキサ/パーサ側でドット展開）。テスト継続中

- ~prelude の値化
  - 状態: 実装済（`Prelude.ls` を ~builtins/~internal を一時注入して評価し、値を `prelude` に束縛）

- 未束縛診断
  - 状態: 実装済（定義位置と包含チェーンを出力）

- include の字句実装と循環検出
  - 状態: 実装済（エラーメッセージは yyerror 経由、内→外順チェーン）

- エラー時の終了コード
  - 状態: 実装済（CLI/ENV でゼロにするオプション有）

- List.ls の安定化
  - 状態: 進行中（`t53_list_basic` 回帰調査中）

## テスト状況

- 影響テスト
  - `t59_prelude_autodiscover`: 旧 prelude$builtin 依存のため落ち（後方互換削除方針に合わせ調整予定）
  - `t70_lambda_choice_in_nslit`, `t75_double_map_in_nslit`: スキップ（テスト再有効化予定）

## 今後の対応

- NSLIT 関連テストの再有効化と期待値更新
- List.ls の未束縛回帰の修正
- ドキュメント（`docs/05_syntax_sugar.md` 他）の整合性チェック

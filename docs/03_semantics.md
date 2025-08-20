# セマンティクス (Semantics)

本処理系は遅延評価 (call-by-need を志向) のサンク `lsthunk_t` を中心に動作します。トップレベル環境 `lstenv_t` にコアビルトインおよびプレリュードを登録し、式を WHNF まで評価します。

## 値とサンクの種類

`lstype_t` により以下の種類を扱います:
- `ALGE` 代数データ: コンストラクタ名と引数列 (`lsealge_t`)
- `APPL` 関数適用: 関数と引数列
- `CHOICE` 選択: `a | b`
- `INT` 整数
- `STR` 文字列
- `LAMBDA` ラムダ (パラメータパターンと本体)
- `REF` 参照 (`~name`)
- `BUILTIN` ビルトイン関数 (外界へ副作用可能)

主なコンストラクタ関数は `thunk.h` に定義されています（例: `lsthunk_new_ealge`, `lsthunk_new_appl`, `lsthunk_new_int`, `lsthunk_new_str`, `lsthunk_new_elambda`, `lsthunk_new_ref`, `lsthunk_new_builtin`）。

## 環境と参照解決

- トップレベル環境 `lstenv_t` は親を持てる階層構造。
- 参照パターン `~name` は tpat 変換時に `lstref_target` を作成し、環境へ登録されます。
- 評価時、`lsthunk_eval0` が `REF` を解決し、対応するターゲット（ビルトインや束縛）にフォローします。未束縛や型不一致はエラー計上されます。

## パターンマッチ

- 代数データ、as、参照の 3 系列をサポート。
  - `lsthunk_match_alge`, `lsthunk_match_pas`, `lsthunk_match_ref`
- cons `:` とタプル `,`、リスト `[]` は通常の代数データとして統一的に扱われます。
- as パターンは `name@pat` で、`name` にも同じ値をバインドします。

## 評価規則 (概略)

- `INT`/`STR` は値。
- `ALGE` はヘッド（コンストラクタ名）までは値、必要に応じて引数を評価。
- `APPL` は関数を WHNF まで評価してから適用。
  - `BUILTIN` であれば即座に `func(argc,args)` を呼ぶ（厳密さはビルトイン次第）。
  - `LAMBDA` であればパターンマッチで実引数を束縛して本体を評価。
- `CHOICE a|b` は左優先で評価し、失敗した場合に右を試す（実装依存）。

## コアビルトイン

`ls_register_core_builtins` で登録:
- `dump x` → デバッグ表示して `x` を返す
- `to_str x` → 値の pretty 表現を文字列に変換
- `print x` → 値を出力（改行なし）
- `seq a b` → `a` を評価してから `b` を返す
- `seqc a b` → チェーン用の連接（内部用）
- `add x y`, `sub x y` → 整数演算

## プレリュード

- 名前 `prelude` の 1 引数ディスパッチ関数。
- 入力が 0 引数の代数データ（シンボル）であることを期待し、以下のビルトインを返す:
  - `prelude(exit)` → `exit : Int -> ()` に相当（0 なら正常終了）
  - `prelude(println)` → `println : a -> ()` 出力と改行
  - `prelude(chain)` → `chain : Action -> (Unit -> r) -> r` 的な連接
  - `prelude(return)` → 恒等
- 外部プラグインでも同じ契約で差し替え可能。`dlopen` で `ls_prelude_register` を探して呼び出します。

## エラー処理

- 構文・字句エラーは位置情報付きで報告。
- 実行時エラー（未束縛参照、型不一致など）はエラーカウントに計上し、可能な範囲で継続（セマンティクスは今後調整予定）。

## 実装と将来拡張

- 現在は WHNF ベースの簡略化評価器。Strictness や最適化は未実装。
- 失敗/選択の評価戦略、例外・IO モデルは要設計。
- 型システムは未実装（将来的に Hindley–Milner 等の導入を検討）。

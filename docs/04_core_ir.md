# Core IR (暫定設計)

目的:
- 純粋な計算を明示化し、効果の境界を静的/動的に検査しやすくする
- 最適化/将来のJITの受け皿にする（段階的導入）

方針:
- A-Normal Form (ANF) 風の単純IR
- 値/変数/let/ラムダ/適用/構築子/リテラル/条件/効果トークンを区別
- 効果は Capability Token(E) を経由してのみ発生可能とする

## 構文（概念）

- 値 V ::= Int | Str | Constr C [V*] | Var x | Lam (p) . e
- 式 e ::= V | let x = a in e | App V [V*] | If V then e else e | EffApp V E
- 代入不可、変数は単一代入（SSAに近い）
- a（計算） ::= PureOp(V, ...) | Build(C, [V*]) | Closure(Lam, Env) 等

- 効果トークン E は IO などの資源を表す
  - E は `seq/chain` でのみ生成・受け渡し
  - EffApp は (effectful builtin, E, args...) → (E', result) を返す

## 型/効果（簡略）

- τ ::= Int | Str | Constr C [τ*] | τ -> τ | τ -(IO)-> τ
- E は値ではなく、評価器のモード/スレッドローカルで管理可能

## 変換（AST→Core IR）

- 各式を ANF に落とし、サブルーチンに `let` を導出
- ラムダはクロージャ環境を明示化（自由変数の束縛リスト）
- `seq/chain` に相当する箇所で E を導出・伝播

## 実行モデル

- Core IR インタプリタを実装（当面JITなし）
- E が無いコンテキストで EffApp を見つけたらエラー
- E は `seq/chain` 相当のプリミティブにより導入・スレッドセーフに伝搬

## 今後

- β簡約/let 浮上/共通部分式除去など軽量最適化
- ホット関数単位のJIT（ORC v2）
- 正確GC/stackmap は将来検討

## 次の一歩（実装計画）

短期で終わる粒度に分解した具体的アクション。各項目は個別にマージ可能で、段階的に進められる。

1. Core IR スケルトンの拡充（1PR）
  - lscir の最小データ型を定義
    - 値: Int, Str, Constr(name, [args]), Var(id), Lam(param, body)
    - 式: Let(bindings, tail), App(func, [args]), If(cond, then, else)
    - 効果: EffApp(func, token, [args]), Token(opaque)
  - `coreir/coreir.h` に型と基本 API を追加
  - `lscir_print` を人間可読な S式風に拡張

2. AST→Core IR 変換器の足場（2PR）
  - 変換コンテキスト（変数供給, 環境, 位置情報）を導入
  - リテラル/変数/単純適用の ANF 化を実装（ハッピーパス）
  - 最小テストを追加（小さな式を IR に落として文字列比較）

3. ラムダとクロージャ自由変数の扱い（1PR）
  - 自由変数列を抽出し、`Lam` に環境配列を付与
  - `App` 時にクロージャの環境を引き回す形へ変換
  - プリンタとテストを更新

4. 効果トークンの導入と伝播（2PR）
  - フロントエンド（AST）で `seq/chain` を検出し、Core IR の `Token` 生成/受渡に写像
  - `print/println/exit` などの効果内蔵ビルトインを `EffApp` に落とす
  - `--strict-effects` 時に IR 上の静的検査（EffApp が Token のスコープ外に無いか）を追加

5. Core IR インタプリタの最小実装（1PR）
  - β簡約なしで ANF を逐次評価
  - `EffApp` は現ランタイムへブリッジ（既存 thunk/builtin を呼ぶ）
  - スモークテスト（既存 t0x_* を IR 経由でも通す）

6. デバッグ UX（1PR）
  - `--dump-coreir` に AST 併記/カラー化/位置情報の出力を追加
  - 失敗時に当該ノードの IR をトレース表示

7. 最小最適化パス（任意, 小PR）
  - 単純な let 連結・不要 let 除去
  - リテラル畳み込み（add/sub の即値）

マイルストーン目安:
- M1: 1〜3 を完了（IR 出力と基本変換が安定）
- M2: 4〜5 を完了（IR 実行で既存テストをパス）
- M3: 6〜7 を適用（開発体験とパフォーマンス改善）

補足:
- 既存ランタイム（thunk/builtin）と併存運用し、段階的に IR 経由へ切替
- 変換/実行はファイル単位のフラグで有効化し、回帰を防止

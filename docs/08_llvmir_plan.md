# [DEPRECATED] LLVM IR ロードマップ（フェーズ1まで実施、以降は保留）

この計画は廃止されました。今後は LSTR/LSTI ベースの実装方針に一本化します。以下は履歴として残しています。

本ドキュメントは、Core IR から LLVM IR へのロワリング計画です。現状はフェーズ1まで実施し、以降は保留として設計を記します。

## 現状（フェーズ1まで）
- 共有C-ABI呼び出し規約の足場を追加（CoreIR/LLVMIR 共通）
  - `src/coreir/cir_callconv.{h,c}`
- LLVM IR スケルトンライブラリとダンプツール
  - `src/llvmir/`（`llvmir_emit.c` は最小の IR を出力）
  - `src/lsllvmir_dump`（標準入力からパース→Core IR→LLVMテキスト出力）
- テキストIRのAOT動線（環境にclangがあれば .ll → 実行ファイル）

## 今後のフェーズ（保留）

### フェーズ2: 整数のネイティブ化
- i64 をネイティブ値として扱い、加減乗除・比較のIR生成を実装
- 共有ABIとの橋渡し（必要時に `lsrt_make_int` で thunk に戻す）

### フェーズ3: 代数データのタグ化/パターンマッチ
- 安定タグとレイアウト設計（構造体 or tag+payload）
- コンストラクタ生成/アンパックをネイティブ化
- パターンマッチを `switch` による分岐へ
- 遅延評価ルール（外側のみWHNF）を維持（内部は遅延/強制の境界を定義）

### フェーズ4: 関数/クロージャのネイティブ化
- 共有ABIに沿うシグネチャ設計（例: `lsrt_value_t *fn(int argc, lsrt_value_t **args)`）
- キャプチャ環境の表現（構造体+関数ポインタ、またはGC管理オブジェクト）
- `apply` ホットパスの直接呼出し

### フェーズ5: 遅延評価（任意）
- Thunk 構造体+メモ化、必要箇所からコールバイニード化
- 脱糖/最適化での強制点の挿入

### フェーズ6: 効果と周辺
- println 以外の I/O、`require`/`def` のJIT/AOT連携
- 例外/エラー処理の設計

## 使い方メモ

- LLVM IR テキスト出力:
  - `./src/lsllvmir_dump < input.ls > out.ll`
  - `echo "(\\x -> 42);" | ./src/lsllvmir_dump > out.ll`
- AOT（任意）:
  - `clang out.ll -o a.out && ./a.out`

## 注意
- 現段階の `llvmir_emit.c` は最小の雛形（`main` が 0 を返す）です。実際のロワリングはフェーズ2以降で拡張します。

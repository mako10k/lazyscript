# LSTI 移行計画（実行ランタイム）

目的: 実行ランタイムを LSTI（Runtime Thunk Image）対応へ段階的に切替え、同一アーキ上での起動時間とメモリ局所性を改善する。

## 方針
- LSTB（可搬）と併存し、用途で選択可能にする。
- 初期はマテリアライズ方式（イメージ→通常Thunk）で安全導入。後段でゼロコピー・ビューを追加。
- ビルド/実行時フラグで制御し、CI で双方の経路を常時検証。

## フェーズ

### Phase 0: 仕様確定/ガード
- thunk-binary-format.md の LSTI 章を元に、ヘッダ（magic/version/align/flags）とセクション種別IDを確定。
- ビルドフラグ: ENABLE_LSTI（有効化）、実行フラグ: LAZYSCRIPT_IMAGE=1（使用指示）。
- 受け入れ基準: 定数/構造体の合意、ビルドが通る。

### Phase 1: ミニマムエンコーダ
- 対応 kind: INT/STR/SYMBOL/ALGE/BOTTOM。
- セクション: STRING_BLOB/SYMBOL_BLOB/THUNK_TAB/ROOTS（必要最小）。
- アライン/相対オフセット配置、自己検証（境界/整列）実装。
- 受け入れ基準: 生成→検証が成功、破損検出が機能。

### Phase 2: マテリアライズローダ
- LSTI→通常Thunkへの復元（共有/循環の二段再構築）。
- フォールバック: 失敗時 LSTB を試行。
- 受け入れ基準: LSTB→LSTI→ロード→評価がスモークで等価。

### Phase 3: カバレッジ拡大
- APPL, LAMBDA(+PATTERN_TAB), CHOICE(λ/expr/catch), REF(内部/外部/組込), BUILTIN, TYPE_TAB。
- 受け入れ基準: 主要テストが LSTI 経由で緑。

### Phase 4: ゼロコピー・ビュー
- イメージ上ノードを直接評価（相対オフセット解決、WHNFキャッシュは別領域）。
- 受け入れ基準: マテリアライズと等価結果、起動/メモリの優位をベンチで確認。

### Phase 5: テスト/ベンチ/堅牢化
- 単体/プロパティ: ヘッダ/整列/未知セクションスキップ、LSTB↔LSTIラウンドトリップ。
- ベンチ: 起動時間、常駐メモリ、初回評価レイテンシ。
- 受け入れ基準: 劣化なし、または改善が確認できる。

### Phase 6: ロールアウト
- 初期はデフォルト LSTB。CI に LSTI ジョブ追加。
- 安定後にデフォルト切替検討。ドキュメント/リリースノート更新。

## API（案）
- int lsti_write(FILE* out, const ls_program_t* prog, const lsti_write_opts_t* opt);
- int lsti_map(const char* path, lsti_image_t* out_img);
- int lsti_validate(const lsti_image_t* img, lsti_validation_report_t* rep);
- int lsti_materialize(const lsti_image_t* img, ls_program_t** out_prog);
- int lsti_unmap(lsti_image_t* img);

## フラグと切替
- ビルド: -DENABLE_LSTI
- 実行: 環境変数 LAZYSCRIPT_IMAGE=1（既定0）。
- CLI（任意）: --image / --no-image。

## リスクと緩和
- ABI/整列不一致: ヘッダ厳格検証、拒否ポリシー。
- 複雑化: 変換ユーティリティ整備、コード共有（kinds/flags/セクションID）。
- 安全性: 初期はマテリアライズで侵襲を抑制。

## 受け入れ基準（総括）
- 各フェーズの基準を満たすこと。
- CI で LSTB/LSTI 両経路が緑。
- ベンチで改善が確認できること（Phase 4以降）。

## 付記
- 詳細仕様は `docs/thunk-binary-format.md` の LSTI 章を参照。
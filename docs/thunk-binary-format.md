# LazyScript Thunk Binary Format (proposal v0.1)

本書は、LazyScript の実行時 Thunk グラフを永続化/IPC/デバッガ用に安全にバイナリ化するための設計案です。現行ランタイム構造体（`src/thunk/thunk.c`）と言語要素（'|', '||', '^|' を含む）に整合することを目標にします。

- 目的
  - ランタイム Thunk グラフのスナップショット保存/復元
  - デバッガ/トレースビューアへの受け渡し（共有・重複の保存）
  - 将来の分散実行やキャッシュ（WHNF の持ち越し）
- 非目標（v0.1）
  - GC/アロケータ状態の完全再現
  - OS/ポインタ依存のアドレス保持

## 全体方針

- グラフは「オブジェクト ID 参照」で表現（ポインタは使わない）。
- 共有（同一ノードの多重参照）は ID を再利用して保存。循環も可。
- すべて Little-Endian、可変長整数（ULEB128/SLEB128）でサイズ/ID を縮小。
- セクション化（ヘッダ、文字列プール、パターンプール、Thunk テーブル、ルート一覧、オプションのデバッグ情報）。
- 前方互換の拡張を flags とセクション追加で実現。

## 用語とプリミティブ

- u8/u16/u32/u64: Little-Endian の符号なし整数
- i64: Little-Endian の符号付き 64bit 整数
- varuint: ULEB128（可変長）
- varint: SLEB128（可変長）
- id: varuint（0..N-1 のインデックス）
- off32/off64: 相対オフセット（将来拡張用。v0.1 では固定テーブルで不要）

## ファイルレイアウト（v0.1）

```
+----------------------------+
| magic = 'LSTB' (0x4C535442)| 4B
| version_major              | u16 (=0x0001)
| version_minor              | u16 (=0x0001)
| flags                      | u32 (bitfield、下記)
| section_count              | u16 (v0.1 は5以上。追加セクション可)
+----------------------------+
| Section[0]: STRING_POOL    |
| Section[1]: SYMBOL_POOL    |
| Section[2]: PATTERN_POOL   |
| Section[3]: THUNK_TABLE    |
| Section[4]: ROOTS          |
| Section[5]: TYPE_POOL      |  ← 型参照の予約領域（v0.1 追加）
| （将来: DEBUG, TRACE など） |
+----------------------------+
```

- flags（v0.1）
  - bit0: store_whnf_cache（WHNF キャッシュを含める）
  - bit1: store_trace_id（trace_id を含める）
  - bit2: store_locs（Bottom の loc を拡張格納）
  - bit3: store_types（型参照を格納する。未設定なら全エントリに型情報なし）

### STRING_POOL

- 文字列を一意化して格納。UTF-8。
- layout:
  - count: varuint
  - entries: count 回
    - len: varuint
    - bytes: len バイト

### SYMBOL_POOL

- 言語の「シンボル」（先頭に '.' を含む）やコンストラクタ名、参照名等の識別子を格納。
- layout は STRING_POOL と同一（将来、種別タグを追加可能）。

### PATTERN_POOL（パターン AST）

- ラムダ param、バインド lhs、キャレット/OR などのパターンを保存。
- layout:
  - count: varuint
  - entries: count 回、各 entry:
    - kind: u8
      - 0=ALGE, 1=AS, 2=INT, 3=STR, 4=REF, 5=WILDCARD, 6=OR, 7=CARET
    - payload: kind ごとに可変
      - ALGE: constr_sym_id(varuint), argc(varuint), args[argc]=pat_id
      - AS:   ref_pat_id(varuint), inner_pat_id(varuint)
      - INT:  i64（将来: 可変長整数を検討）
      - STR:  str_id(varuint)
      - REF:  name_sym_id(varuint)
      - WILDCARD:（なし）
      - OR: left_pat_id(varuint), right_pat_id(varuint)
      - CARET: inner_pat_id(varuint)（v0.1 は WILDCARD or REF のみ推奨）

### THUNK_TABLE（本体）

- layout:
  - count: varuint
  - entries: count 回、各 entry:
    - kind: u8
      - 0=ALGE, 1=APPL, 2=CHOICE, 3=LAMBDA, 4=REF, 5=INT, 6=STR, 7=SYMBOL,
        8=BUILTIN, 9=BOTTOM
  - flags: u8（ビット: 0=whnf, 1=has_type, 2..7=将来）
    - size: varuint（payload のバイト数。スキップ・拡張用）
    - optional: if (file.flags.store_trace_id) trace_id: varint
  - optional: if (flags & has_type) type_id: varuint（TYPE_POOL 参照）
    - payload: kind ごと

- kind ごとの payload 定義:
  - ALGE:
    - constr_sym_id: varuint
    - argc: varuint
    - args[argc]: thunk_id(varuint)
  - APPL:
    - func_id: varuint
    - argc: varuint
    - args[argc]: thunk_id(varuint)
  - CHOICE:
    - choice_kind: u8（1=lambda '|', 2=expr '||', 3=catch '^|'）
    - left_id: varuint
    - right_id: varuint
  - LAMBDA:
    - param_pat_id: varuint（PATTERN_POOL 参照）
    - body_id: varuint
  - REF:
    - mode: u8（0=bound-in-graph, 1=external-by-name, 2=builtin-by-name）
    - when mode==0: target_thunk_id: varuint
    - when mode==1: name_sym_id: varuint
    - when mode==2: name_sym_id: varuint（ロード時に ~prelude 等から解決）
  - INT:
    - value: varint（SLEB128、64bit 範囲）
  - STR:
    - str_id: varuint（STRING_POOL 参照）
  - SYMBOL:
    - sym_id: varuint（SYMBOL_POOL 参照）
  - BUILTIN:
    - name_sym_id: varuint
    - arity: varuint
    - attr: u16（`lsbuiltin_attr_t` のビット写像、未知ビットは無視）
  - BOTTOM:
    - msg_str_id: varuint（0 は空文字に予約推奨）
    - loc_mode: u8（0=unknown, 1=short, 2=full）
      - short: file_str_id(varuint), line(u32), col(u32)
      - full:  reserved（将来の拡張）
    - related_count: varuint
    - related[related_count]: thunk_id(varuint)

備考:
- `lt_whnf` はポインタなので保存しない。flags.whnf=1 の場合は「このノードは WHNF 済み」情報のみ保持。
- `trace_id` は任意。ロード側で新規採番に差し替えることも可。
- 型参照: file.flags.store_types=1 の時のみ有効。エントリごとに has_type=1 なら `type_id` を持つ。

### ROOTS（エントリポイント）

- 複数のトップレベル Thunk を公開したいケース向け。
- layout:
  - root_count: varuint
  - roots[root_count]: thunk_id(varuint)

### TYPE_POOL（型参照の予約領域）

- 目的: 将来の型システム導入に備え、各 Thunk に型参照を付与できるようにする。
- v0.1 では型の解釈は未定。ここでは「識別子や将来のblobへの参照」を格納する最小仕様のみ提供。
- layout:
  - type_count: varuint
  - types[type_count]: entry
    - kind: u8
      - 0=STRING_NAME（STRING_POOL の文字列 ID を指す）
      - 1=SYMBOL_NAME（SYMBOL_POOL のシンボル ID を指す）
      - 2=OPAQUE_BLOB（将来用の任意バイト列。現行は読み飛ばしのみ）
    - payload: kind ごと
      - STRING_NAME: str_id(varuint)
      - SYMBOL_NAME: sym_id(varuint)
      - OPAQUE_BLOB: blob_len(varuint), blob_bytes[blob_len]

## 互換性戦略

- version_major が変わる変更は後方互換なし。minor/flags/追加セクションでの追加は後方互換ありを原則。
- 未知 kind/未知 attr ビット/未知セクションは、size フィールドでスキップ可能に設計。
- 参照解決（REF mode==1/2）は、ロード時の環境（`~prelude` 等）に依存。解決不可はエラーかプレースホルダ Bottom 作成のどちらかを選択可能に。
- TYPE_POOL は存在しなくてもよい（file.flags.store_types=0 の場合）。存在しても、各エントリの has_type=0 なら `type_id` は現れない。

## セキュリティと整合性

- 入力境界で全 varuint の上限チェック、size による境界超過検知。
- 循環は許容（例: Y コンビネータ）。ロード時は 2 フェーズ（割当→辺解決）で構築。
- ID 範囲・重複参照の検証、未定義参照はエラーに。

## 例（抜粋）

- INT 42 のみのグラフ:
  - STRING_POOL: count=0
  - SYMBOL_POOL: count=0
  - PATTERN_POOL: count=0
  - THUNK_TABLE:
    - count=1
    - entry0: kind=INT, flags=0, size=..., value=42
  - ROOTS: [0]

- CHOICE '^|' の例: `E ^| (\\^x -> H)`（右は lamchain の先頭ラムダ）
  - left = thunk_id=0（E）
  - right = thunk_id=1（lambda param=CARET(REF x), body=H）
  - entry2: kind=CHOICE, choice_kind=3, left_id=0, right_id=1

## ランタイム対応（実装メモ）

- writer:
  - 走査で DAG を ID 化（`lsthunk_colle_new` の要領）。
  - 文字列/シンボル/パターンの事前収集とプール採番。
  - kind ごとに payload を直列化。
- reader:
  - テーブルサイズ確定後に `lsmalloc` で Thunk 配列を確保（`lt_whnf=NULL` 初期化）。
  - 第1段: kind に応じて union メンバのコンテナを確保・基本フィールド設定。
  - 第2段: 参照 ID を解決してポインタ接続。REF 外部は環境に問い合わせ。
  - 必要なら WHNF フラグに応じて `lt_whnf=自分` をセット（INT/STR/SYMBOL/ALGE など）。

## 拡張の余地

- 大整数/バイナリ Blob（将来の数値型）
- 位置情報の高精度化（ファイル ID, スパン、トレーススタック）
- 圧縮（セクション単位の zstd）
- 署名/ハッシュ（整合性検証）

## 付録: 現行ランタイムとの対応表（要約）

- LSTTYPE_ALGE  ↔ kind=ALGE
- LSTTYPE_APPL  ↔ kind=APPL
- LSTTYPE_CHOICE↔ kind=CHOICE（choice_kind=1/2/3）
- LSTTYPE_LAMBDA↔ kind=LAMBDA（param=PATTERN_POOL）
- LSTTYPE_REF   ↔ kind=REF（bound/external/builtin）
- LSTTYPE_INT   ↔ kind=INT
- LSTTYPE_STR   ↔ kind=STR
- LSTTYPE_SYMBOL↔ kind=SYMBOL
- LSTTYPE_BUILTIN↔ kind=BUILTIN
- LSTTYPE_BOTTOM↔ kind=BOTTOM

この案は最小限の実装コストで現行機能（'^|' の caret パターン、Bottom の関連引数、右結合 choice）をカバーします。v0.1 ではパターンの完全表現と REF の 2 モード（内部/外部）を優先し、将来の相互運用に備えてセクション/size による前方互換を確保します。

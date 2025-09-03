# LSTR: LazyScript Thunk Readable IR（COREIR 再設計案）

目的
- 互換性制約を外して設計を再出発し、LSTI/LSTB と相互変換しやすい「実行直前IR」を提供する。
- LSTI の表現力（LAMBDA のパターン、CHOICE、BOTTOM、REF など）を素直に受け止め、後段（JIT/LLVMIR）へ安定供給。
- テキストで読み書き可能（Readable）かつ、実装は最小限の構成。

非目標
- 旧 Core IR（LCIR）の互換維持は行わない（必要なら薄いアダプタを別途用意）。

---

## データモデル（v1）
値（Value）
- int: 64bit（将来拡張可）
- str: UTF-8（所有権は外部に任せ文字列テーブル参照も可）
- ref: 名前参照（シンボル）
- constr: コンストラクタ適用（名前 + 値配列）
- lam: パターン引数のラムダ（param はパターン。body は式）
- bottom: 実行不能（msg と関連ノード）
- choice: λ/expr/catch の二択

式（Expr）
- val v
- let x = e1 in e2（ANF 補助として暫定）
- app f(vs)
- match v with (pat -> expr)+
- effapp f(vs, token?) / token（将来用）

パターン（Pat）
- wildcard / var name / constr name subpats[] / int n / str s / as ref_pat inner / or p1 p2 / caret p

---

## テキスト表現（s-expr 風）
例:
- (int 42)
- (str "abc")
- (ref prelude/println)
- (constr Cons (var x) (constr Nil))
- (lam (constr Pair (var a) (var b)) (var a))
- (app (ref add) (int 1) (int 2))
- (match (var v)
    ((constr Nil)                      (constr Zero))
    ((constr Cons (var h) (var t))     (app (ref f) (var h) (var t))))
- (bottom "msg" (var ctx1) (var ctx2))
- (choice lambda (lam (var x) (var x)) (lam (var y) (var y)))

ヘッダ: "; LSTR v1" を先頭コメントとして許容。

---

## LSTI/LSTB との相互変換（要点）
- int/str/ref/constr/lam/app/bottom/choice は 1:1 マップ。
- match は LSTR 内で保持。LSTI 側では既存ノードへ落とし込み（ラムダ/APPL 等）で代替。
- パターンは LSTI PATTERN_TAB（ALGE/AS/INT/STR/REF/WILDCARD/OR/CARET）と損失なく往復。
- LSTR→LSTI では STRING_BLOB/SYMBOL_BLOB へ集約、PATTERN_TAB を生成。

---

## API/配置（提案）
- ディレクトリ: src/lstr/
- ファイル: lstr.h, lstr_print.c, lstr_text_reader.c, lstr_validate.c,
            lstr_to_thunk.c, thunk_to_lstr.c
- 関数例:
  - const lstr_prog_t* lstr_read_text(FILE* in, FILE* err);
  - void lstr_print(FILE* out, int indent, const lstr_prog_t* p);
  - int lstr_validate(const lstr_prog_t* p);
  - int lstr_to_lsti(FILE* out, const lstr_prog_t* p);
  - const lstr_prog_t* lstr_from_lsti(const char* path);

---

## 導入ステップ
1) LSTR テキスト I/F（read/print/validate）を追加（COREIR と共存）。
2) thunk_to_lstr（LSTI→LSTR）を先に実装して可視化。
3) lstr_to_thunk（LSTR→LSTI/LSTB）を追加し、LSTI 往復を確立。
4) CHOICE/BOTTOM/パターン網羅を拡充。
5) CI に LSTR ラウンドトリップを追加。

## 既知の検討点
- match の LSTI 側へのエンコード簡約
- 効果/トークンのv1の扱い（通過/無視最小）
- 例外と bottom/catch の統合ポリシー
- 名前空間の正規化（SYMBOL_BLOB と整合）

---

この文書は初版ドラフトです。実装と共に更新します。
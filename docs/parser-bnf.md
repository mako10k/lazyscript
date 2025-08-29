# parser.y の BNF（要約）

このファイルは `src/parser/parser.y` に定義された構文を BNF 風に要約したものです。

## 概要
- 出力: BNF 形式（簡潔化、構文部分のみ、semantic actions は含まず）。
- いくつかのトークン（LSTPRELUDE_* など）は字句解析器の追加情報と結びついており、糖（syntactic sugar）はパーサのアクション内で展開されます。

## トークン（端末記号）
- LSTINT : 整数リテラル
- LSTSTR : 文字列リテラル
- LSTSYMBOL : シンボル/識別子（コンストラクタ等）
- LSTPRELUDE_SYMBOL : プレリュード糖 (~~symbol のみ)
- LSTREFSYM : 参照記法トークン（~x 形式）
- LSTCARET : キャレット '^'（Bottom 関連の構文で使用）
- LSTLEFTARROW : "<-"
- LSTARROW : "->"
- LSTWILDCARD : ワイルドカード '_'
- 記号: ';', ':', '|', '\\', '{','}','(',')','[',']','@','!','=', ',' など

## 主要文法（BNF 風）
以下は parser.y から抽出・簡潔化した BNF 形式の規則です。

<prog> ::= <expr> ';'

<bind> ::= <bind_list>

<funlhs> ::= <pref>

<bind_single> ::= <funlhs> <lamparams> '=' <expr>
                | <pat> '=' <expr>

<bind_list> ::= ';' <bind_single>
              | <bind_list> ';' <bind_single>

<expr> ::= <expr1>

<expr1> ::= <expr2>
          | <lamchoice>

<expr2> ::= <expr3>
          | <econs>

<econs> ::= <expr1> ':' <expr3>

        <expr> ::= <expr1>
        <expr1> ::= <expr2>
          | <expr4> '||' <expr3>

<eappl> ::= <efact> <expr5>
          | <eappl> <expr5>

<expr4> ::= <expr5>
          | <ealge>

<ealge> ::= LSTSYMBOL <expr5>
          | <ealge> <expr5>

<expr5> ::= <efact>
        <expr4> ::= <expr5>
                  | <eappl>
                  | <ealge>
                  | <lamchain>
/* efact: 基本要素 */
<efact> ::= LSTINT
         | LSTSTR
         | LSTREFSYM
         | LSTPRELUDE_SYMBOL
        <efact> ::= LSTINT
         | <etuple>
         | <elist>
         | <closure>
         | <elambda>
         | '!' '{' <dostmts> '}'
         | '{' <nslit_entries> '}'

        /* lambda */
        <elambda> ::= '\\' <lamparams> LSTARROW <expr3>

        /* lambda-only choice (right associative) */
        <lamchain> ::= <elambda>
                     | <elambda> '|' <lamchain>
/* do-block スタイル（dostmt: 中間ノード） */
         優先度: expr 系は複数レベルに分かれ、適用 (application), アルゲ (algebraic constructor), 選択 (choice '|' / '||'), cons ':' 等の優先度が分離されている。
         ‘|’ はラムダ専用（<lamchain>）で右結合。一般式のフォールバックは ‘||’ を使用。
         ラムダを適用ヘッドに置くには括弧が必要（例: `\\x -> f x y` に引数を適用する場合は `((\\x -> f x y) a)` のように括弧で囲む）。

<dostmt> ::= <pref> LSTLEFTARROW <expr>
           | LSTSYMBOL LSTLEFTARROW <expr>
           | <expr>

/* closure */
<closure> ::= '(' <expr> <bind> ')'

/* tuple / array / list */
<etuple> ::= '(' ')'
          | '(' <earray> ')'

<earray> ::= <expr>
          | <earray> ',' <expr>

<elist> ::= '[' ']'
         | '[' <earray> ']'

/* lambda */
<elambda> ::= '\\' <lamparams> LSTARROW <expr2>

/* lambda-only choice (right associative) */
<lamchoice> ::= <lamfactor>
              | <lamfactor> '|' <lamchoice>

<lamfactor> ::= <elambda>
              | '(' <lamchoice> ')'

/* lambda パラメータ（パターン） */
<lamparam> ::= <lamparam2>
             | <lamparam2> '|' <lamparam>

<lamparam2> ::= LSTSYMBOL
              | <pat3>
              | '(' <pat> ')'

<lamparams> ::= <lamparam>
             | <lamparams> <lamparam>

/* pattern */
<pat> ::= <pat1>
        | <pcons>

<pcons> ::= <pat> ':' <pat>

<pat1> ::= <pat2>
         | <pat2> '|' <pat1>

<pat2> ::= <pat3>
         | <palge>

<palge> ::= LSTSYMBOL
         | <palge> <pat3>

<pat3> ::= <ptuple>
         | <plist>
         | LSTINT
         | LSTSTR
         | <pref>
         | LSTWILDCARD
         | '^' '(' <pat> ')'
         | <pref> '@' <pat3>

/* ref */
<pref> ::= LSTREFSYM

/* tuple/array/list（pattern 側） */
<ptuple> ::= '(' ')'
          | '(' <parray> ')'

<parray> ::= <pat>
           | <parray> ',' <pat>

<plist> ::= '[' ']'
         | '[' <parray> ']'

## 実装メモ
- 2025-08: `+` は仕様外（削除済み）。`~~` 糖は `~~symbol` のみに統一。
- 2025-08: 識別子(ident)の許容文字は `[a-zA-Z_][a-zA-Z0-9_]*` に限定。`$` は不可。必要な場合はクォートしたコンストラクタ `'ident$'` を用いる。
- parser.y 内の多くの規則は semantic action によって "糖" を展開（ns リテラル、do-block、リスト/タプルのデシュガーなど）する。
- 優先度: expr 系は複数レベルに分かれ、適用 (application), アルゲ (algebraic constructor), 選択 (choice '|' / '||'), cons ':' 等の優先度が分離されている。
- ラムダ本体は <expr2>（‘|’ の一段上）にしてあるため、`|` はラムダより弱い（`\x -> a | b` は `(\x -> a) | b` とは解釈されない）。
- このドキュメントは文法の読みやすさ優先で簡潔化しているため、細部の AST 構築/位置情報などは省略している。
 - 2025-08: '^' の導入
         - パターン側: `^(<pat>)` は Bottom 値にのみマッチする特殊パターン。
         - 式側: `^(<expr>)` は Bottom を構築する（専用 AST ノードを生成して評価時に構築）。
         - 仕様詳細は `docs/spec/bottom-and-caret.md` を参照。

## 参照
- 元ソース: `src/parser/parser.y`

---
作成日時: 自動生成 (parser.y から要約)

# lazyscript

実験的な遅延評価スクリプト系言語の処理系です。Autotools でビルドできます。

## ビルド

依存:
- bdw-gc
- flex (2.6+)
- bison (3.x)
- libtool, autoconf, automake

```
./configure
make -j
sudo make install   # 任意
```

## 実行

```
./src/lazyscript --help
./src/lazyscript -e "..."
```

### 主なCLIオプションと環境変数

- 実行/デバッグ系:
  - `-e, --eval <prog>`: 1行プログラムを評価
  - `-i, --dump-coreir`: パース後の Core IR を表示
  - `-c, --eval-coreir`: Core IR 評価器で実行（スモーク）
  - `-s, --strict-effects`: 効果の順序付け規律を検証（`chain/seq` 必須）
  - `-t, --typecheck`: Core IR の最小タイプチェック（OK / E: type error を出力）
  - `--no-kind-warn`: 効果（IO種別）の警告を抑制（既定は警告を stderr に出力）
  - `--kind-error`: 効果（IO種別）をエラーとして扱う（stderr にエラーを出力し非ゼロ終了）
- プラグイン/糖衣:
  - `-p, --prelude-so <path>`: prelude プラグイン .so を指定（未指定時は組み込み）
  - `-n, --sugar-namespace <ns>`: `~~sym` の展開先名前空間を指定（既定 `prelude`）
- 環境変数:
  - `LAZYSCRIPT_PRELUDE_SO`: `--prelude-so` と同義の上書き
  - `LAZYSCRIPT_SUGAR_NS`: `--sugar-namespace` と同義の上書き

詳細は以下も参照:
- シンタックスシュガーと do 記法: `docs/05_syntax_sugar.md`
- 最小タイプチェッカ: `docs/06_typecheck.md`
- 型/値コンストラクタの分離とIO規律（設計メモ）: `docs/07_effect_types.md`

## コアビルトインとプレリュード

lazyscript には以下の2層があります:
- コアビルトイン: `dump`, `to_str`, `print`, `seq`, `seqc`, `add`, `sub` など。常に組み込み登録されます。
- プレリュード: `prelude` ディスパッチ関数で、`exit`, `println`, `chain`, `return` などを提供します。既定では組み込み版が登録されますが、外部 .so で差し替え可能です。

### プレリュードの差し替え

- 使い方:
  - 環境変数 `LAZYSCRIPT_PRELUDE_SO` で .so のパスを指定
  - もしくは CLI オプション `--prelude-so <path>` を指定
- ランタイムは `dlopen(<path>)` → `dlsym("ls_prelude_register")` を呼び、成功時はプラグイン内の登録関数が `prelude` を環境に登録します。失敗時は組み込みプレリュードにフォールバックします。

登録関数のシグネチャ:

```
int ls_prelude_register(lstenv_t *tenv);
```
- 戻り値 0 で成功、それ以外で失敗。

## サンプル: デフォルトプレリュード (.so)

`src/plugins` 配下に、組み込みと同等の機能を持つサンプルプラグインを用意しています。
ビルドすると `liblazyscript_prelude.so` としてインストールされます。

- 提供関数:
  - `prelude(exit)` → 組み込み `exit`
  - `prelude(println)` → 文字列化して改行出力
  - `prelude(chain)` → アクションを実行後、`()` を引数に継続を適用
  - `prelude(return)` → 引数をそのまま返す

詳しくは `doc/PLUGINS.md` も参照してください。

### 使い方

```
# ビルド
./configure && make -j

# サンプルプラグインを使用
./src/lazyscript -p ./src/plugins/.libs/liblazyscript_prelude.so -e '...'
# もしくは
LAZYSCRIPT_PRELUDE_SO=/path/to/liblazyscript_prelude.so ./src/lazyscript -e '...'
```

ヒント: インストール先は `plugindir = $(libdir)/lazyscript` です。`make install` 後は `$(libdir)/lazyscript/` に配置されます。

## 開発メモ
- プラグインからは `thunk/thunk.h`, `thunk/tenv.h`, `expr/ealge.h`, `common/str.h`, `common/io.h` を参照します。
- ホストバイナリのシンボルを解決するため、`lazyscript` は `-export-dynamic` でリンクされています。

## ランタイム拡張（自己ホストの土台）

起動時に初期化スクリプトを読み込む/実行時にスクリプトをロードすることで、言語自身での拡張を可能にします。

- `--init <file>` / `LAZYSCRIPT_INIT`
  - 起動後、ユーザコードの前に LazyScript を 1 度評価します（thunk 実装経路）。
  - `strict-effects` 有効時は副作用コンテキスト内で評価されます。

- `~~require "path.ls"`
  - 実行時に LS ファイルを読み込み・評価します。
  - `LAZYSCRIPT_PATH`（コロン区切り）を検索し、同じパスは 1 回だけロードされます（簡易キャッシュ）。

- `~~def Name value`
  - 現在の環境に `Name` を束縛します（トップレベル相当）。以降 `~Name` で参照可能。
  - 例: `{ ~~def foo 123; ~~println (~to_str ~foo) };  # => 123`

メモ: 参照の解決は評価時にも行われるため、`def` や `require` で後から導入された名前も参照可能です。

## LLVM IR（スケルトン）

Core IR から LLVM IR へのロワリング準備として、最小構成を追加しています。

- 共有C-ABI呼び出し規約: `src/coreir/cir_callconv.{h,c}`
- LLVM IR ライブラリ: `src/llvmir/`（最小のテキスト出力）
- ダンプツール: `src/lsllvmir_dump`

使い方（例）:

```
echo "(\\x -> 42);" | ./src/lsllvmir_dump > out.ll
```

詳細なロードマップは `docs/08_llvmir_plan.md` を参照してください。

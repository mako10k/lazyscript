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

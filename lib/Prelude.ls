(
  # Prelude 評価時にのみ注入される ~builtins を取り込み、その上で公開
  { # 純粋プレリュード: 値として公開
  .chain      = (~builtins .chain);
  .bind       = (~builtins .bind);
  .return     = (~builtins .return);
  .eq         = (~builtins .eq);
  .lt         = (~builtins .lt);
  .add        = (~builtins .add);
  .sub        = (~builtins .sub);
  .to_str     = (~builtins .to_str);
  .nsMembers  = (~builtins .nsMembers);
  # .include 廃止（字句レイヤーの {- #include -} を使用）
  # namespace literal support: prelude 経由の委譲は廃止（コア実装に委ねる）
  # 標準ライブラリ再エクスポート（後置 let で定義される ~List を公開）
  .List       = ~List;
  .Option     = ~Option;
  .Result     = ~Result;
  .String     = ~String;
  # 汎用ユーティリティ（純粋; 非正格評価を前提に簡潔に記述）
  .flip       = (\~f -> \~x -> \~y -> ~f ~y ~x);
  # 代替の自己適用コンビネータ（Y風）。代入構文を避けてパーサ互換性を確保
  .fix        = (\~f -> (\~x -> ~f (~x ~x)) (\~x -> ~f (~x ~x)));
  .id         = (\~x -> ~x);
  .const      = (\~x -> \_ -> ~x);
  # 環境を変更する内部 API（Prelude 評価時のみ有効な ~internal から引き出し）
  .env = {
  .require     = (~internal .require);
  .requireOpt  = (~internal .requireOpt);
  .println     = (~builtins .println);
  .print       = (~builtins .print)
  };
  }
  ;
  # 末尾で純粋 include により標準ライブラリを取り込み（式としてインライン展開し束縛）
  ~List   = {- #include "lib/List.ls" -};
  ~Option = {- #include "lib/Option.ls" -};
  ~Result = {- #include "lib/Result.ls" -};
  ~String = {- #include "lib/String.ls" -}
)

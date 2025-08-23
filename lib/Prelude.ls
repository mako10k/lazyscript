(
  # Prelude 評価時にのみ注入される ~builtins を取り込み、その上で公開
  { # 純粋プレリュード: 値として公開
  .println    = (~builtins .println);
  .chain      = (~builtins .chain);
  .bind       = (~builtins .bind);
  .return     = (~builtins .return);
  .print      = (~builtins .print);
    .eq         = (~builtins .eq);
    .lt         = (~builtins .lt);
  .add        = (~builtins .add);
  .sub        = (~builtins .sub);
  .to_str     = (~builtins .to_str);
  .nsMembers  = (~builtins .nsMembers);
  .nsSelf     = (~builtins .nsSelf);
  # nslit$N はプリミティブ経由で利用（値プレリュードでは未公開）
  # 環境を変更する内部 API（Prelude 評価時のみ有効な ~internal から引き出し）
  .env = {
    .require     = (~internal .require);
    .requirePure = (~internal .requirePure);
    .import      = (~internal .import);
    .withImport  = (~internal .withImport);
    .def         = (~internal .def);
    .nsnew       = (~internal .nsnew);
    .nsdef       = (~internal .nsdef);
    .nsnew0      = (~internal .nsnew0);
    .nsdefv      = (~internal .nsdefv);
    .nsMembers   = (~internal .nsMembers);
    .nsSelf      = (~internal .nsSelf)
  };
  }
)

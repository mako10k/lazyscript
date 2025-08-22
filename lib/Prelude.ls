{
  # 純粋プレリュード: 高階ユーティリティを値として提供
  # 実装は ~builtin 経由でホストのビルトインに委譲（環境更新は提供しない）
  .println    = (~builtin println);
  .chain      = (~builtin chain);
  .bind       = (~builtin bind);
  .return     = (~builtin return);
  .nsMembers  = (~builtin nsMembers);
  .nsSelf     = (~builtin nsSelf);
  .nslit$2    = (~builtin nslit$2);
  .nslit$4    = (~builtin nslit$4);
  .nslit$6    = (~builtin nslit$6)
}

{
  # 集約モジュール（純）: 主要 lib をまとめて参照提供
  .List   = ((~prelude requirePure) "lib/List.ls");
  .Option = ((~prelude requirePure) "lib/Option.ls");
  .Result = ((~prelude requirePure) "lib/Result.ls");
  .Ns   = ((~prelude requirePure) "lib/Ns.ls");
  .String= ((~prelude requirePure) "lib/String.ls")
};

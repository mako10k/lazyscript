{
  # 集約モジュール（純）: 主要 lib をまとめて参照提供
  .List   = ((~builtin requirePure) "lib/List.ls");
  .Option = ((~builtin requirePure) "lib/Option.ls");
  .Result = ((~builtin requirePure) "lib/Result.ls");
  .Ns     = ((~builtin requirePure) "lib/Ns.ls");
  .String = ((~builtin requirePure) "lib/String.ls")
};

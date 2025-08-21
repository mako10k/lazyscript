{
  # 集約モジュール（純）: 主要 lib をまとめて参照提供
  .List   = ((~prelude requirePure) "lib/List.ls");
  .Option = ((~prelude requirePure) "lib/Option.ls");
  .Result = ((~prelude requirePure) "lib/result.ls");
  .ns     = ((~prelude requirePure) "lib/ns.ls");
  .string = ((~prelude requirePure) "lib/string.ls")
};

!{
  ~R <- ((~prelude .requirePure) "lib/Result.ls");
  ~~println (~to_str (((~R .Result) .Ok) 1));
  ~ok2 <- (((~R .Result) .Ok) 2);
  ~mapinc <- ((~R .map) (\~x -> (~add ~x 1)));
  ~~println (~to_str ((~mapinc ~ok2)));
  ~err <- (((~R .Result) .Err) "e");
  ~withDef <- ((~R .withDefault) 9);
  ~~println (~to_str ((~withDef ~err)));
};

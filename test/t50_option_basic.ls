!{
  ~M <- ((~prelude .requirePure) "lib/Option.ls");
  ~~println (~to_str ((~M .Option) .Some 1));
  ~opt2 <- ((~M .Option) .Some 2);
  ~inc  <- ((~M .map) (\~x -> (~add ~x 1)));
  ~~println (~to_str ((~inc ~opt2)));
  ~none <- ((~M .Option) .None);
  ~getOr <- ((~M .getOrElse) 99);
  ~~println (~to_str ((~getOr ~none)));
};

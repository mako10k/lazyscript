!{
  ~M <- ((~prelude requirePure) "lib/Option.ls");
  ~~println (~to_str (((~M .Option) Some) 1));
  ~~println (~to_str (((~M .map) (\~x -> ~add ~x 1)) (((~M .Option) Some) 2)));
  ~~println (~to_str ((((~M .getOrElse) 99) (((~M .Option) None)))));
};

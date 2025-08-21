!{
  ~M <- ((~prelude .requirePure) "test/pure_mod.ls");
  ~~println (~to_str ((~M .Foo)));
  ~~println (~to_str (((~M .Bar) 41)));
};

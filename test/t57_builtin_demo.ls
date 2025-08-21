!{
  ~mod <- ((~prelude .builtin) "demo");
  ~~println (~to_str (((~mod .hello))));
  ~~println (~to_str (((~mod .add1) 41)));
};

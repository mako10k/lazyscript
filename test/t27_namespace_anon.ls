!{
  ns <- ((~prelude nsnew0));
  ((~prelude nsdefv) ~ns Foo 42);
  ~~println (~to_str ((~ns Foo)))
};

!{
  ns <- ((~prelude nsnew0));
  ((~ns __set) Foo 42);
  ~~println (~to_str ((~ns Foo)))
};

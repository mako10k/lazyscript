!{
  ns <- ((~prelude nsnew0));
  ((~prelude nsdefv) ~ns .k 1);
  ((~prelude nsdefv) ~ns "s" 2);
  ((~prelude nsdefv) ~ns 123 3);
  ((~prelude nsdefv) ~ns Foo 4);
  ~~println (~to_str ((~ns .k)));
  ~~println (~to_str ((~ns "s")));
  ~~println (~to_str ((~ns 123)));
  ~~println (~to_str ((~ns Foo)));
};

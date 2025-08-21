!{
  ~ns <- ((~prelude .nsnew0));
  ~~nsdefv ~ns .Foo 42;
  ~~println (~to_str ((~ns .Foo)))
};

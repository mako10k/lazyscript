!{
  ~~nsnew NS;
  NS <- { .Foo = 1; };  # replace NS binding by literal value (immutable)
  ((~prelude nsdefv) ~NS .Bar 2)  # should error (immutable)
};

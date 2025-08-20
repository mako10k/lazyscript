!{
  ns <- { .Foo = 1; };
  ((~prelude nsdefv) ~ns .Bar 2)  # should error (immutable)
};

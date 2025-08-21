!{
  ns <- { .Foo = 1; };
  ((~ns .__set) .Foo 2)  # should error (immutable)
};

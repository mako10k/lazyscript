!{
  ns <- { .Foo = 1; };
  # Attempt to use setter on immutable namespace should error
  ((~ns .__set) .Bar 2)
};

!{
  NS <- { .Foo = 1; };  # immutable value
  ((~NS .__set) .Bar 2)
};

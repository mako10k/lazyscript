!{
  ~ns <- { .Foo = 1; };
  ~~nsdefv ~ns .Bar 2  # should error (immutable)
};

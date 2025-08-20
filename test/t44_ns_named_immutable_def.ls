!{
  ~~nsnew NS;
  NS <- { .Foo = 1; };  # replace NS binding by literal value (immutable)
  ~~nsdefv ~NS .Bar 2  # should error (immutable)
};

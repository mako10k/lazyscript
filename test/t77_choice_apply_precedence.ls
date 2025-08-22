!{
  ~~println (~to_str (((\true -> 1 | \false -> 2) true)));
  ~~println (~to_str (((\true -> 1 | \false -> 2) false)));
  ~~println (~to_str (((((\true -> 1) | (\false -> 2)) true))));
  ~~println (~to_str (((((\true -> 1) | (\false -> 2)) false))));
};

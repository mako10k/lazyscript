!{
  # Avoid ambiguity by writing as nested lambdas
  ~~println (~to_str (((\ (Cons ~x) -> (\ ~y -> ~x)) (Cons 7)) 0));
};

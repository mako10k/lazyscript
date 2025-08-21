!{
  f <- (\ ~x -> ~add ~x 1);
  map <- (\ ~f -> \ ~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt);
  ~~println (~to_str ((~map ~f) (Some 2)));
};

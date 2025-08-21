(
  NS <- {
    .Option <- ({ .None = None; .Some = \~x -> Some ~x });
    .map    <- (\~f -> \~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt);
  };
  ((~prelude println) (~to_str ((((NS .map) (\~x -> ~add ~x 1)) (((NS .Option) Some) 2)))));
)

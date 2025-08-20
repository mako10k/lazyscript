{
  .Result = (!!{ .Ok = \~x -> Ok ~x; .Err = \~e -> Err ~e });
  .map = (\~f -> \~r -> (
    \(Ok ~x) -> Ok (~f ~x) | \(Err ~e) -> Err ~e
  ) ~r);
  .flatMap = (\~f -> \~r -> (
    \(Ok ~x) -> ~f ~x | \(Err ~e) -> Err ~e
  ) ~r);
  .withDefault = (\~d -> \~r -> (
    \(Ok ~x) -> ~x | \(Err _) -> ~d
  ) ~r)
};

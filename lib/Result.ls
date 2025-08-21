{
  .Result      = ({ .Ok = (\~x -> (Ok ~x)); .Err = (\~e -> (Err ~e)) });
  .map         = (\~f -> \~r -> (
    !{
      ~m <- (\(Ok ~x) -> (Ok (~f ~x)) | \(Err ~e) -> (Err ~e));
      (~m ~r)
    }
  ));
  .flatMap     = (\~f -> \~r -> (
    !{
      ~m <- (\(Ok ~x) -> (~f ~x) | \(Err ~e) -> (Err ~e));
      (~m ~r)
    }
  ));
  .withDefault = (\~d -> \~r -> (
    !{
      ~m <- (\(Ok ~x) -> ~x | \(Err ~e) -> ~d);
      (~m ~r)
    }
  ))
};

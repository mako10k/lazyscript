{
  .Option = ({ .None = None; .Some = (\~x -> (Some ~x)) });
  .map = (\~f -> \~opt -> (
    !{
      ~m <- (\(Some ~x) -> (Some (~f ~x)) | \None -> None);
      (~m ~opt)
    }
  ));
  .flatMap = (\~f -> \~opt -> (
    !{
      ~m <- (\(Some ~x) -> (~f ~x) | \None -> None);
      (~m ~opt)
    }
  ));
  .getOrElse = (\~d -> \~opt -> (
    !{
      ~m <- (\(Some ~x) -> ~x | \None -> ~d);
      (~m ~opt)
    }
  ))
};

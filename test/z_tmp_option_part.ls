{
  .Option = ({ .None = None; .Some = \~x -> Some ~x });
  .map = (\~f -> \~opt -> (
    \None -> None | \(Some ~x) -> Some (~f ~x)
  ) ~opt);
};
